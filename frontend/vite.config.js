import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

export default defineConfig({
  plugins: [vue()],
  server: {
    port: 5173,
    host: '0.0.0.0',
    proxy: {
      '/ws': {
        target: 'ws://localhost:8081',
        ws: true,
        changeOrigin: true
      },
      '/static': {
        target: 'http://localhost:8080',
        changeOrigin: true
      }
    }
  }
})
