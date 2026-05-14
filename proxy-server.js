const net = require('net');
const http = require('http');
const fs = require('fs');
const path = require('path');
const WebSocket = require('ws');

const BACKEND_HOST = '127.0.0.1';
const BACKEND_PORT = 8080;
const WS_PORT = 8081;
const HTTP_PORT = 8082;
const STATIC_DIR = path.join(__dirname, 'backend', 'static');

// ===== HTTP 静态文件服务器 =====
const httpServer = http.createServer((req, res) => {
  // 处理静态文件请求（头像等）
  if (req.url.startsWith('/static/')) {
    const filePath = path.join(STATIC_DIR, req.url.replace('/static/', ''));
    
    // 安全检查：防止路径遍历
    if (!filePath.startsWith(STATIC_DIR)) {
      res.writeHead(403);
      res.end('Forbidden');
      return;
    }
    
    fs.readFile(filePath, (err, data) => {
      if (err) {
        res.writeHead(404);
        res.end('Not Found');
        return;
      }
      
      // 设置正确的 Content-Type
      const ext = path.extname(filePath).toLowerCase();
      const contentTypes = {
        '.png': 'image/png',
        '.jpg': 'image/jpeg',
        '.jpeg': 'image/jpeg',
        '.gif': 'image/gif',
        '.svg': 'image/svg+xml',
        '.webp': 'image/webp'
      };
      
      res.writeHead(200, {
        'Content-Type': contentTypes[ext] || 'application/octet-stream',
        'Access-Control-Allow-Origin': '*'
      });
      res.end(data);
    });
  } else {
    res.writeHead(404);
    res.end('Not Found');
  }
});

httpServer.listen(HTTP_PORT, () => {
  console.log(`HTTP static server running on http://localhost:${HTTP_PORT}`);
  console.log(`Serving static files from: ${STATIC_DIR}`);
});

// ===== WebSocket 代理服务器 =====
const wss = new WebSocket.Server({ port: WS_PORT });

console.log(`WebSocket proxy server running on ws://localhost:${WS_PORT}`);
console.log(`Forwarding to ${BACKEND_HOST}:${BACKEND_PORT}`);

wss.on('connection', (ws) => {
  console.log('New WebSocket client connected');
  
  const tcpClient = new net.Socket();
  let receiveBuffer = Buffer.alloc(0);
  
  tcpClient.connect(BACKEND_PORT, BACKEND_HOST, () => {
    console.log('Connected to backend TCP server');
  });
  
  tcpClient.on('data', (data) => {
    receiveBuffer = Buffer.concat([receiveBuffer, data]);
    
    while (receiveBuffer.length >= 4) {
      const msgLength = receiveBuffer.readUInt32BE(0);
      
      if (receiveBuffer.length < 4 + msgLength) {
        break;
      }
      
      const msgData = receiveBuffer.slice(0, 4 + msgLength);
      receiveBuffer = receiveBuffer.slice(4 + msgLength);
      
      ws.send(msgData);
    }
  });
  
  tcpClient.on('error', (err) => {
    console.error('TCP connection error:', err);
    ws.close();
  });
  
  tcpClient.on('close', () => {
    console.log('TCP connection closed');
    ws.close();
  });
  
  ws.on('message', (message, isBinary) => {
    let buffer;
    if (Buffer.isBuffer(message)) {
      buffer = message;
    } else if (message instanceof ArrayBuffer) {
      buffer = Buffer.from(message);
    } else if (Array.isArray(message)) {
      buffer = Buffer.concat(message);
    } else {
      buffer = Buffer.from(message);
    }
    
    if (buffer.length > 0) {
      tcpClient.write(buffer);
    }
  });
  
  ws.on('close', () => {
    console.log('WebSocket client disconnected');
    tcpClient.destroy();
  });
  
  ws.on('error', (err) => {
    console.error('WebSocket error:', err);
    tcpClient.destroy();
  });
});
