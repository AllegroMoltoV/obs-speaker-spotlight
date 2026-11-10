# 前提の検証 V1・V2: StreamKit の実際の DOM と立ち絵の置き換え

## はじめに

計画の A2 の結果です。要件 `.prompts/INIT.md` の 5.2 と 5.3 は、StreamKit のスクリプトを読んで推定した DOM の構造を前提にしています。この前提を、実際の通話を表示したページで確かめました。あわせて、立ち絵の置き換えの仮決めの方式を確かめました。OBS は起動していません。

## 検証環境

- Chrome 157.0.8103.39 のヘッドレス。起動の引数は `--headless=new --remote-debugging-port=9333 --disable-features=LocalNetworkAccessChecks` と、専用の `--user-data-dir`
- Discord のデスクトップアプリ。依頼者が 1 人で音声チャンネルに入った状態
- StreamKit のスクリプトは `main.daa4bcc9.js` です。
- 調べる道具は Node 24 の WebSocket で CDP に接続するスクリプトです。`.tmp/2026111009-cdp-eval.mjs`、`2026111012-cdp-trace.mjs`、`2026111014-watch-speaking.mjs`、`2026111017-v2-content.mjs` に置きました。

## 検証結果

### Chrome から Discord のクライアントへの接続

ページは `ws://127.0.0.1:6463` から `6472` へ順に WebSocket で接続を試みます。オプションを付けない Chrome 157 では、`6463` への接続が次のエラーで失敗し、参加者の一覧は空のままでした。

```
WebSocket connection to 'ws://127.0.0.1:6463/?v=1&client_id=207646673902501888' failed: Error in connection establishment: net::ERR_BLOCKED_BY_LOCAL_NETWORK_ACCESS_CHECKS
```

`--disable-features=LocalNetworkAccessChecks` を付けて起動し直すと接続でき、`AUTHENTICATE`、`GET_CHANNEL`、5 種類のイベントの `SUBSCRIBE` が成功しました。購読したイベントは `VOICE_STATE_CREATE`・`VOICE_STATE_DELETE`・`VOICE_STATE_UPDATE`・`SPEAKING_START`・`SPEAKING_STOP` です。

### V1: 実際の DOM

参加者 1 人の `li` の `outerHTML` は次のとおりです。ユーザー ID とアバターのハッシュと名前は伏せています。

```html
<li class="Voice_voiceState__OCoZh is_widget_owner voice_state" data-userid="<ユーザー ID>">
  <img class="Voice_avatar__htiqH voice_avatar" src="https://cdn.discordapp.com/avatars/<ユーザー ID>/<ハッシュ>.jpg" alt="">
  <div class="Voice_user__8fGwX voice_username">
    <span class="Voice_name__TALd9" style="color: rgb(255, 255, 255); font-size: 14px; background-color: rgba(30, 33, 36, 0.95);">(名前)</span>
  </div>
</li>
```

依頼者に 90 秒間、話す・黙るを繰り返してもらい、100 ms ごとに `li` と `img` のクラスを読みました。

| 観測 | 結果 |
| --- | --- |
| 発話の開始の回数 | 10 回 |
| 発話中の `li` のクラス | `Voice_voiceState__OCoZh wrapper_speaking is_widget_owner voice_state` |
| 発話中の `img` のクラス | `Voice_avatar__htiqH Voice_avatarSpeaking__lE+4m voice_avatar` |
| `wrapper_speaking` と `Voice_avatarSpeaking__lE+4m` の付き外れ | 10 回とも同じ時点。片方だけが付いた状態は観測されなかった |
| 発話と待機の切り替わりの最短の間隔 | 0.1 秒。0.1〜0.6 秒の間隔で切り替わる場面が 4 か所あった |

構造は INIT.md の 5.2 の表のとおりでした。

### V2: `content` プロパティによる立ち絵の置き換え

100 x 200 の png を canvas で作りました。上半分が rgb(200, 100, 40)、下半分が rgb(40, 200, 100) です。data URI の長さは 1,474 文字です。次の CSS を `style` 要素として `head` に注入しました。obs-browser と同じ注入の方法です。

```css
.voice_state .voice_avatar { filter: brightness(0.5); transition: filter 0s; }
.voice_state.wrapper_speaking .voice_avatar { filter: brightness(1); }
.voice_state[data-userid="<ユーザー ID>"] .voice_avatar {
  content: url("data:image/png;base64,...");
  width: auto; height: 200px; border-radius: 0; border: 0;
}
```

`li` の `wrapper_speaking` を CDP で付け外しし、スクリーンショットの画素を読みました。

| 状態 | 計算済みの `filter` | 描画の大きさ | 上半分の画素 | 下半分の画素 |
| --- | --- | --- | --- | --- |
| 待機 | `brightness(0.5)` | 100 x 200 | (100, 50, 20) | (20, 100, 50) |
| 発話 | `brightness(1)` | 100 x 200 | (200, 100, 40) | (40, 200, 100) |
| 待機に戻す | `brightness(0.5)` | 100 x 200 | (100, 50, 20) | (20, 100, 50) |

計算済みの `content` は `url("data:image/png;base64,...")` で、`border-radius` は `0px` でした。スクリーンショットは `.tmp/2026111018-v2-idle.png` と `2026111018-v2-speaking.png` にあります。

### 保存した DOM

ローカルのページの元として、次を保存しました。

- `.tmp/2026111015-streamkit-idle.json`: 待機中の `document.documentElement.outerHTML` と、読み込まれたスタイルシートの本文
- `.tmp/2026111016-streamkit-speaking.html`: 発話中の `#root` の `outerHTML`

## 考察

- 発話中の判定には、ハッシュの無い `wrapper_speaking` を使えると考えています。ハッシュ付きのクラスと同時に付き外れするため、ハッシュ付きのクラスを併用しても判定の精度は上がらないと推測します。
- 0.1 秒の間隔の切り替わりは、要件 6.2 が挙げた雑音によるちらつきに当たると考えています。切り替えの時間の設定の既定値を、0 より大きくする必要があると思います。
- `content` プロパティの方式は、Chrome 157 で仮決めの確定の条件を満たしています。OBS 33.0.1 の CEF 150 でも同じになると推測しますが、B1 で確かめます。
- Chrome の Local Network Access の制限が OBS の CEF にも入ると、StreamKit がクライアントに接続できなくなると考えています。OBS 33.0.1 の CEF 150 の挙動は B1 で確かめます。

## 未確認

- アバターを設定していない参加者の `img` の `src` と、その参加者での置き換え。参加者が 1 人のため確かめていません。B3 で確かめます。
- 参加者が 2 人以上のときの並びと、ほかの参加者の `wrapper_speaking`。
- OBS 33.0.1 の CEF 150 での V1・V2 の結果と、Discord のクライアントへの接続。
