/// <reference types="vite/client" />

// WebView2 chrome.webview API 타입 정의
interface ChromeWebView {
  postMessage(message: any): void;
  addEventListener(
    type: 'message',
    listener: (event: { data: any }) => void
  ): void;
  removeEventListener(
    type: 'message',
    listener: (event: { data: any }) => void
  ): void;
}

interface Window {
  chrome?: {
    webview?: ChromeWebView;
  };
}
