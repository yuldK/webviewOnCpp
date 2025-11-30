import React, { useCallback, useEffect, useMemo, useState } from "react";

const isWebView2 = Boolean(window.chrome?.webview);

type MessagePayload = Record<string, unknown>;

function postToCpp(message: MessagePayload) {
  if (!isWebView2) {
    console.warn("WebView2 환경이 아니어서 postMessage를 건너뜁니다.", message);
    return;
  }

  window.chrome!.webview!.postMessage(message);
  console.log("📤 C++로 전송:", message);
}

function formatMessage(value: unknown) {
  try {
    return JSON.stringify(value, null, 2);
  } catch (err) {
    return String(value ?? "");
  }
}

const App: React.FC = () => {
  const [lastMessage, setLastMessage] = useState<string>("대기중...");
  const [isReady, setIsReady] = useState<boolean>(false);

  const statusText = useMemo(
    () => (isWebView2 ? "WebView2 연결됨" : "브라우저 모드 (postMessage 미지원)"),
    []
  );

  const sendPing = useCallback(() => {
    postToCpp({
      type: "ping",
      payload: "hello from React",
      timestamp: new Date().toISOString(),
    });
  }, []);

  useEffect(() => {
    if (!isWebView2) return undefined;

    const listener = (event: { data: unknown }) => {
      setLastMessage(formatMessage(event.data));
    };

    const webview = window.chrome!.webview!;
    webview.addEventListener("message", listener);

    // Notify C++ that UI is ready
    postToCpp({
      type: "ready",
      payload: "UI loaded successfully",
    });
    setIsReady(true);

    return () => {
      webview.removeEventListener("message", listener);
    };
  }, []);

  return (
    <div className="app-shell">
      <header className="app-header">
        <div>
          <p className="eyebrow">WebView on C++</p>
          <h1>Vite + React UI</h1>
          <p className="subtitle">
            WebView2 메시지를 React 컴포넌트로 처리합니다. 아래 버튼을 눌러 C++로 ping을 보낼 수 있습니다.
          </p>
        </div>
        <div className={`status-chip ${isReady ? "active" : "idle"}`}>
          {statusText}
        </div>
      </header>

      <section className="card">
        <div className="card-header">
          <div>
            <p className="eyebrow">Interop</p>
            <h2>C++로 메시지 보내기</h2>
            <p className="subtitle">ping 메시지를 보내고, 응답을 아래에서 확인하세요.</p>
          </div>
          <button className="primary" onClick={sendPing} disabled={!isWebView2}>
            C++로 ping 보내기
          </button>
        </div>
      </section>

      <section className="card">
        <div className="card-header">
          <div>
            <p className="eyebrow">Latest message</p>
            <h2>C++에서 받은 데이터</h2>
          </div>
        </div>
        <pre className="message-view">{lastMessage}</pre>
      </section>
    </div>
  );
};

export default App;
