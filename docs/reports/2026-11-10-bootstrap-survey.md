# 初回調査: 名前の重複、StreamKit の版、兄弟プラグインからの流用

## はじめに

`.prompts/INIT.md` の要件を受けて、着手前に確かめるべき事実を調べた結果です。INIT.md で確認済みの事実は繰り返しません。調査日は 2026-11-10 です。

## 検証結果

### 名前の重複

| 確認先 | 方法 | 結果 |
| --- | --- | --- |
| GitHub | `gh search repos obs-voice-spotlight` | 0 件 |
| GitHub | `gh search repos voice-spotlight` | 20 件。OBS のプラグインは無い |
| 倉庫くん | MCP の `search_plugins` で `spotlight`、`スポットライト`、`Discord` | いずれも 0 件 |

### StreamKit の版

`curl https://streamkit.discord.com/overlay/voice/0/0` は HTTP 200 を返しました。HTML が参照するファイルは `static/js/main.daa4bcc9.js` と `static/css/main.32bcc709.css` です。INIT.md の 5.2 と同じ版です。HTML は `.tmp/2026111001-streamkit-voice.html` に置きました。

### OBS のコアのシグナル

`docs/sphinx/reference-core.rst` の master ブランチに、次のシグナルが記載されています。INIT.md の 12 節で未確認だった項目です。

- `source_create` (ptr source)
- `source_destroy` (ptr source)
- `source_remove` (ptr source)
- `source_rename` (ptr source, string new_name, string prev_name)

### 兄弟プラグイン obs-comment-skin の状態

`D:/work/obs-comment-skin` をサブエージェントが読み取り専用で調べました。コマンドは `git log --oneline`、`git status --short`、`git remote -v`、`bd list --all`、`bd memories --json`、`bd show` です。

- 段階 c と d の実装と、依頼者のユーザーテストは終わっています。HEAD は 255a683 です。
- 0.1.0 と 1.0.0 は未公開で、GitHub のリモートも未設定です。
- 公開は、P0 の不具合 obs-comment-skin-dxs.24 の修正を待っています。OBS の終了時に、ドック内のプレビューへ CSS を送るとクラッシュする不具合です。原因は未特定です。

確定済みの設計判断のうち、本プロジェクトに関係するものは次のとおりです。

| 事項 | comment-skin の判断 | 根拠 |
| --- | --- | --- |
| `bundleId` | `jp.allegromoltov.obs-comment-skin` | 親課題の design |
| ソースごとの設定 | `obs_source_get_private_settings()` | OBS 33.0.1 の `libobs/obs.c` 2510 行・2366 行、`libobs/obs-source.c` 709 行 |
| プリセット | `obs_module_config_path("presets.json")` | 同上 |
| 候補の一覧の更新 | シグナルを使わず、一覧を開くときとシーンコレクションの切り替えで作り直す | `src/comment-skin-dock.cpp` 101〜105 行 |
| 適用 | 設定を変えるたびに 150 ms 後に自動で適用する | memory `auto-apply-decision` |
| プレビュー | OBS のプレビューで確かめる方式から、ドック内の QCef の見本へ依頼者の決定で変更 | memory `dock-preview-decision` |
| ドックの文言 | 操作の説明文を置かない。見本ではなくプレビューと書く | memory `ui-no-explanatory-text`、`ui-term-preview` |
| 依頼者に渡す手順書 | `.prompts/MANUALS/<yyyymmddnn>-<名前>.md` | memory `manuals-location` |

comment-skin で実機確認済みの事実は次のとおりです。環境は OBS 33.0.1 のポータブル版、CEF 150.0.7871.252、obs-browser 2.27.5 です。

- private settings は、終了後のシーンの JSON に保存され、再起動の後に復元されました。シーンコレクションの切り替えとソースの複製の後の挙動は、実機では未確認です。
- `css` を書き換えるとページが読み込み直されます。書き換えから表示への反映は 785〜881 ms でした。この値は 150 ms の待ち時間を含みます。
- 開発中の検証は、ポータブル版の `plugins/<名前>/` に DLL を直下に置く形式で行っています。ドックは UI Automation で操作し、ページの中は `--remote-debugging-port=9222` の CDP で調べています。

### 流用の区分

| ファイル | 区分 |
| --- | --- |
| `src/core/managed-css.*` | そのまま写せる。目印の文字列だけを変える |
| `src/plugin-main.cpp`、`src/source-state.*`、`src/preset-store.*`、`src/create-source-dialog.*`、`src/core/css-builder.h` | 名前と中身を直して写す |
| `src/settings-widgets.*` | 折りたためる区画、色、スライダーはほぼそのまま写せる |
| `src/core/style-schema.*` | 型とサニタイズの部分を写す。項目の表は写さない |
| `src/preview-panel.*`、`deps/obs-browser/`、`data/preview/` | 写さない。P0 のクラッシュの経路であるため |
| `src/core/chat-target.*`、`youtube-style.*`、`twitch-style.*` | 写さない。URL の分解の関数だけを警告の判定に流用できる |
| `tests/core-tests.cpp` | 仕組みを写す。フレームワークを使わない `CHECK` マクロの単体テスト |
| `CMakeLists.txt`、`CMakePresets.json`、`.github/workflows/`、`cmake/windows/helpers.cmake`、`.gitignore` | comment-skin の形を写す。obs-loudness-balance-monitor との差分は、ctest の追加と、OBS 33 の配置の形式への対応 |

画像のファイル選択、data URI の生成、ユーザー ID の検証、URL の警告は、comment-skin にありません。新規に書きます。

ロゴは、comment-skin では SVG の 3 案を `.appendix/logo-proposals/` に作り、依頼者が 1 案を選びました。PNG への変換は Chrome のヘッドレスのスクリーンショットで行っています。

## 考察

comment-skin で依頼者が適用を自動に変え、プレビューをドック内に変えたことは、本プロジェクトの INIT.md 6.5 の適用ボタンと 6.7 のプレビューの仮決めに影響すると考えています。ただし本プロジェクトの CSS は立ち絵の data URI を含んで大きく、変更のたびの再読み込みは負担が大きいと推測します。

## 未確認

- 実際の通話を表示した StreamKit の DOM。
- 大きな data URI を含む CSS の扱い。comment-skin にも記録がありません。
- 音声ウィジェットの再読み込みの途切れの長さ。
- private settings の、シーンコレクションの切り替えと複製の後の実機の挙動。
