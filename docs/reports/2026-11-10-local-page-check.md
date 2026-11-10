# A4-3: 保存した DOM のローカルのページでの表示の確認

## はじめに

Beads の課題 obs-voice-spotlight-5dt.3.3 の結果です。A4-1 の `generate_css` が作る CSS を、A2 で保存した StreamKit の DOM から作ったローカルのページに注入しました。要件 `.prompts/INIT.md` の 10 節のうち、CSS の表示に関わる項目を確かめています。OBS は起動していません。

結論として、default と min の設定では確認した項目がすべて期待どおりでした。max の設定では、名前の `span` が立ち絵に重なり、`li` の外へはみ出す不具合を見つけました。修正の案は考察の節に書きます。

## 検証環境

- Chrome 157.0.8103.39 のヘッドレス。起動の引数は `--headless=new --remote-debugging-port=9334` と、専用の `--user-data-dir` の `.tmp/2026111026-chrome-profile` です。
- ページの大きさは CDP の `Emulation.setDeviceMetricsOverride` で 800 x 600 にしました。OBS のブラウザーソースの既定の大きさです。既定の背景色は `Emulation.setDefaultBackgroundColorOverride` で透明にしました。
- CSS は `build_x64/RelWithDebInfo/voice-spotlight-css-dump.exe` で生成しました。実行ファイルの更新日時は 2026-11-10 23:38:50 です。`src/core/voice-style.cpp` の最後の変更はコミット df8159d で、作業ツリーでの変更はありません。

## 検証方法

### ローカルのページ

`.tmp/2026111020-build-local-page.mjs` で、`.tmp/2026111015-streamkit-idle.json` から `.tmp/2026111021-local-page.html` を作りました。StreamKit のスクリプトの `script` 要素を除き、保存したスタイルシートの本文を `style` 要素として埋め込んでいます。外部の読み込みを防ぐため、`Content-Security-Policy` の `meta` 要素で `img-src data:` と `style-src 'unsafe-inline'` だけを許しました。

参加者は次の 3 人です。並びは StreamKit と同じ名前の比較順で、DOM の上から (b)、(c)、(a) です。

| 参加者 | ユーザー ID | アバターの `src` | 立ち絵の登録 |
| --- | --- | --- | --- |
| (a) | 保存した依頼者の ID | 赤の 128 x 128 の SVG の data URI | 登録する。60 x 120 の画像。上半分が橙、下半分が緑。表示の順は 2 番目 |
| (b) | 200000000000000002 | 紫の 128 x 128 の SVG の data URI | 登録しない |
| (c) | 300000000000000003 | `https://cdn.discordapp.com/embed/avatars/0.png` | 登録する。90 x 120 の画像。上半分が青、下半分が黄。表示の順は 1 番目 |

(c) はアバターを設定していない参加者の `src` の形にしました。この URL は CSP で遮断され、読み込まれていません。

### CSS と注入

css-dump に (a) と (c) のユーザー ID を渡し、`.tmp/2026111022-css/` に `default.css`、`max.css`、`min.css` を書きました。3 つの設定の内容は次のとおりです。

| 項目 | default | max | min |
| --- | --- | --- | --- |
| 待機中の明るさ | 50% | 200% | 0% |
| 発話中の明るさ | 100% | 200% | 0% |
| 切り替えの時間 | 200 ms | 2,000 ms | 0 ms |
| 緑の枠 | 表示 | 非表示 | 表示 |
| 立ち絵とアイコンの高さ | 150 px | 600 px | 20 px |
| 名前 | 表示。書体などの指定なし | 表示。Yu Gothic UI、72 px、文字色 rgb(255, 230, 0)、背景色 rgb(20, 20, 60) | 非表示 |
| 並べ方と間隔 | 縦、8 px | 横、200 px | 縦、0 px |

`.tmp/2026111023-local-check.mjs` が、設定ごとにページを読み込み直し、CSS を `style` 要素として `head` に足します。obs-browser と同じ注入の方法です。比較のため、CSS を注入しない状態も記録しました。

### 状態の切り替えと測定

各設定で次の 4 つの状態を作り、計算済みのスタイルと `getBoundingClientRect` とスクリーンショットの画素を記録しました。

1. 待機中
2. (a) だけに `wrapper_speaking` を付けた状態。実際の DOM と同じく、`img` にも `Voice_avatarSpeaking__lE+4m` を付けました。付けてから切り替えの時間の半分の時点で、(a) の `filter` の計算値も読みました。
3. (b) だけに同じクラスを付けた状態。登録していない参加者の緑の枠を確かめるためです。
4. クラスを外して待機中に戻した状態

画素は、`img` の中心の列で、高さの 25% と 75% の位置から読みました。800 x 600 の外にある要素の画素は `captureBeyondViewport` で文書全体を撮って読みました。結果は `.tmp/2026111025-local-check-result.json` にあります。依頼者のユーザー ID は `<ID_A>` に置き換えています。

## 検証結果

### 確認した項目ごとの結果

| 項目 | default | max | min |
| --- | --- | --- | --- |
| (a) の置き換え | 立ち絵になった。`content` は data URI。描画は 75 x 150 で、縦横比 0.5 を保つ | 立ち絵になった。300 x 600 | 立ち絵になった。10 x 20 |
| (c) の置き換え | 立ち絵になった。112.5 x 150 で、縦横比 0.75 を保つ | 立ち絵になった。450 x 600 | 立ち絵になった。15 x 20 |
| 立ち絵の円の切り抜き | `border-radius` は 0px、`border` は 0px | 同じ | 同じ |
| (b) のアバター | `content` は `normal`、`src` のまま。円形。156 x 156 | 同じ。606 x 606 | 同じ。26 x 26 |
| 待機中の明るさ | 3 人とも `brightness(0.5)` | 3 人とも `brightness(2)` | 3 人とも `brightness(0)` |
| (a) の発話中 | (a) だけが `brightness(1)`、(b) と (c) は `brightness(0.5)` | 3 人とも `brightness(2)`。設定値が待機中と同じ | 3 人とも `brightness(0)`。設定値が待機中と同じ |
| (b) の発話中 | (b) だけが `brightness(1)` | 3 人とも `brightness(2)` | 3 人とも `brightness(0)` |
| 待機中に戻す | 3 人とも `brightness(0.5)` | 3 人とも `brightness(2)` | 3 人とも `brightness(0)` |
| `transition` | `filter 0.2s`。半分の時点の (a) は `brightness(0.901202)` | `filter 2s` | `filter`。時間は 0s |
| (b) の発話中の緑の枠 | `3px solid rgb(59, 165, 59)`。画素も (59, 165, 59) | `rgba(0, 0, 0, 0)`。画素の透明度は 0 | `rgb(59, 165, 59)`。画素は `brightness(0)` で (0, 0, 0) |
| 名前の表示の有無 | 表示 | 表示 | `.voice_username` が `display: none` |
| 名前の書体・大きさ・色・背景色 | 指定なし。`style` 属性の値のまま | `"Yu Gothic UI"`、72px、rgb(255, 230, 0)、rgb(20, 20, 60)。`style` 属性に勝つ | 非表示のため対象外 |
| 並べ方 | `flex-direction: column` | `row` | `column` |
| 間隔 | `li` の間が 8 px | `li` の間が 200 px | `li` の間が 0 px |
| 表示の順 | 上から (c)、(a)、(b)。`order` は 1、2、3 | 左から (c)、(a)、(b) | 上から (c)、(a)、(b) |
| body の背景 | `rgba(0, 0, 0, 0)`。要素の外の画素の透明度は 0 | 同じ | 同じ |
| 800 x 600 からのはみ出し | なし | あり。文書の大きさは 1,771 x 727 | なし |
| 要素の重なり | なし | 3 人とも名前の `span` が自分の立ち絵に高さ 35 px 重なる | なし |

### 画素

立ち絵の画素は次のとおりです。値は RGB です。

| 設定と状態 | (a) の上 | (a) の下 | (c) の上 | (c) の下 | (b) |
| --- | --- | --- | --- | --- | --- |
| default の待機中 | (115, 60, 20) | (20, 80, 45) | (25, 45, 110) | (120, 105, 30) | (100, 0, 100) |
| default の (a) の発話中 | (230, 120, 40) | (40, 160, 90) | (25, 45, 110) | (120, 105, 30) | (100, 0, 100) |
| default の (b) の発話中 | (115, 60, 20) | (20, 80, 45) | (25, 45, 110) | (120, 105, 30) | (200, 0, 200) |
| max の各状態 | (255, 240, 80) | (80, 255, 180) | (100, 180, 255) | (255, 255, 120) | (255, 0, 255) |
| min の各状態 | (0, 0, 0) | (0, 0, 0) | (0, 0, 0) | (0, 0, 0) | (0, 0, 0) |

CSS を注入しない状態では、(c) の `img` の位置の画素は透明でした。`src` が CSP で遮断されたためです。

### 外部への読み込み

各設定の読み込みで記録した要求は、ページの `file:` の URL、data URI、`https://cdn.discordapp.com/embed/avatars/0.png`、スタイルシートの `@font-face` の `file:///D:/static/media/...` です。data URI 以外はすべて `blockedReason: csp` で失敗しました。

### max の重なりの詳細

max の待機中の (a) の矩形は次のとおりです。

| 要素 | y | 高さ |
| --- | --- | --- |
| `li` | 16 | 692 |
| `img` | 16 | 600 |
| `.voice_username` | 624 | 84 |
| 名前の `span` | 581 | 146 |

`span` の上端の 581 は `img` の下端の 616 より上にあり、下端の 727 は `li` の下端の 708 より下にあります。(b) と (c) では `.voice_username` の高さが 42 px、`span` の高さが 104 px でした。StreamKit のスタイルシートには `.Voice_voiceContainer__adk9M{...line-height:19px}` と `.Voice_name__TALd9{...padding:4px 6px}` があります。生成した CSS は名前の `line-height` と `display` を指定していません。

(a) の名前は 2 行に折り返されていました。横並びの 3 人の幅の合計が `ul` の幅 785 px を超え、`li` が縮んだためです。

### 重なりの再現と修正の案の確認

修正の案を確かめるため、`.tmp/2026111029-css-variant/` に次の 3 つの CSS を作り、`.tmp/2026111028-variant-check.mjs` で注入しました。修正の案は、名前の `span` に次の宣言を足すものです。

```css
.voice_state .voice_username [class*="Voice_name__"] { display: inline-block !important; line-height: normal !important; }
```

| CSS | 内容 | 重なり |
| --- | --- | --- |
| `max-fix.css` | max に修正の案を足した | なし |
| `minbig.css` | min から名前の `display: none` を除き、名前を 72 px にした。高さ 20 px、縦並び、間隔 0 px | (a) の `span` と (b) の `img` が 8 x 13 px、`span` どうしが最大 199.8 x 69 px 重なる。(c) の `span` の上端は y = -31 |
| `minbig-fix.css` | `minbig.css` に修正の案を足した | なし |

`max-fix.css` では、名前の `span` が `.voice_username` と同じ矩形になり、`li` の高さが `span` を含む値に伸びました。

### スクリーンショット

| ファイル | 内容 |
| --- | --- |
| `.tmp/2026111024-none-idle.png` | CSS を注入しない状態 |
| `.tmp/2026111024-default-idle.png`、`-speaking-a.png`、`-speaking-b.png` | default の待機中、(a) の発話中、(b) の発話中 |
| `.tmp/2026111024-max-idle.png`、`-speaking-a.png`、`-speaking-b.png` | max の同じ 3 状態。800 x 600 の範囲 |
| `.tmp/2026111024-min-idle.png`、`-speaking-a.png`、`-speaking-b.png` | min の同じ 3 状態 |
| `.tmp/2026111030-variant-max-fix.png`、`-minbig.png`、`-minbig-fix.png` | 重なりの再現と修正の案 |

## 考察

- 名前の重なりの原因は、StreamKit の `line-height: 19px` が名前の行に残ることだと考えています。インラインの `span` の大きな文字と縦の余白は行の高さを押し広げないため、`span` の背景が行の外へはみ出すと推測します。StreamKit の元の表示では `.voice_username` の `padding-top: 18px` がこのはみ出しを吸収していたと思います。生成した CSS はこの余白を 0 にしています。
- 重なりが起きる条件は、名前の文字が大きく、かつ横並びの間隔が狭いときか、縦並びで立ち絵の高さが名前の高さより低いときだと考えています。default の 14 px の文字では、はみ出しが約 4 px で、間隔の 8 px に収まったと推測します。
- 修正の案は、名前の `span` に `display: inline-block` と `line-height: normal` を付けることです。`voice-style.cpp` の名前の表示の分岐で、書体などの指定の有無にかかわらず出す形が良いと思います。単体テストには、名前を表示するときに 2 つの宣言が出ることを足す案があります。
- 立ち絵を登録した参加者には `border: 0` が付くため、緑の枠の表示の設定は立ち絵に影響しません。要件 6.4 の緑の枠が立ち絵にも求められるものと解釈すると、設定が表示のときに立ち絵へ `border: 3px solid transparent` を残す変更が要ると思います。採否は開発のセッションの判断だと考えています。
- max の設定では、3 人の横並びが 800 px に収まらず、(b) が画面の外に出ます。設定の上限の組み合わせによるもので、CSS の不具合ではないと考えています。ドックで上限の組み合わせへの注意を出す要否は、B3 の依頼者の確認で決める事柄だと思います。
- max と min は待機中と発話中の明るさが同じ値のため、明るさの切り替えの確認には使えません。切り替えは default で確かめました。

## 未確認

- OBS 33.0.1 の CEF 150 での表示。B1 と B2 で確かめます。
- 実際の Discord の通話での、2 人以上の参加者の `wrapper_speaking` の付き方と並び。
- (c) の `src` を実際に読み込めた状態での置き換え。今回は読み込みを遮断した状態だけを確かめました。
- 実際の StreamKit の `gg sans` などの書体を読み込んだ状態での名前の大きさ。今回は書体の読み込みを遮断しています。
- 名前の書体などを一部だけ指定した組み合わせと、横並びで既定の高さの組み合わせ。
