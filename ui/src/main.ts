import './style.css';

// WebView2 환경 확인
const isWebView2 = !!window.chrome?.webview;

if (!isWebView2) {
  console.warn('⚠️ WebView2 환경이 아닙니다. 일부 기능 비활성화');
}

// C++로 메시지 전송
function sendMessageToCpp(message: any) {
  if (isWebView2) {
    window.chrome!.webview!.postMessage(message);
    console.log('📤 C++로 전송:', message);
  } else {
    console.log('🔶 [시뮬레이션] C++로 전송:', message);
  }
}

// C++에서 메시지 수신
function setupMessageListener() {
  if (isWebView2) {
    window.chrome!.webview!.addEventListener('message', (event) => {
      console.log('📥 C++에서 수신:', event.data);
      displayMessage(event.data);
    });
  }
}

// 메시지를 화면에 표시
function displayMessage(data: any) {
  const display = document.getElementById('messageDisplay');
  if (display) {
    const formatted = JSON.stringify(data, null, 2);
    display.textContent = formatted;
  }
}

// 버튼 클릭 이벤트
document.getElementById('sendBtn')?.addEventListener('click', () => {
  sendMessageToCpp({
    type: 'ping',
    payload: 'hello from js',
    timestamp: new Date().toISOString()
  });
});

// 초기화
setupMessageListener();
console.log('✅ UI 초기화 완료');

// 페이지 로드 시 C++에 알림
if (isWebView2) {
  window.addEventListener('DOMContentLoaded', () => {
    sendMessageToCpp({
      type: 'ready',
      payload: 'UI loaded successfully'
    });
  });
}
