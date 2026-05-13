const net = require('net');
const WebSocket = require('ws');

const BACKEND_HOST = '127.0.0.1';
const BACKEND_PORT = 8080;
const WS_PORT = 8081;

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
    // 处理Buffer或ArrayBuffer格式的消息
    let buffer;
    if (Buffer.isBuffer(message)) {
      buffer = message;
    } else if (message instanceof ArrayBuffer) {
      buffer = Buffer.from(message);
    } else if (Array.isArray(message)) {
      buffer = Buffer.concat(message);
    } else {
      // 尝试转换为Buffer
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
