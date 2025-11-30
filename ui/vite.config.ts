import { defineConfig } from 'vite';

export default defineConfig({
  base: './',  // ⚠️ 중요: file:/// 로딩을 위한 상대 경로 설정
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
    rollupOptions: {
      output: {
        entryFileNames: 'assets/[name]-[hash].js',
        chunkFileNames: 'assets/[name]-[hash].js',
        assetFileNames: 'assets/[name]-[hash].[ext]'
      }
    }
  }
});
