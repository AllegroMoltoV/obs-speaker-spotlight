# 文書索引

context compaction で文脈が失われた場合は、この索引から必要な文書をたどる。

| 知りたいこと | 参照先 |
|---|---|
| プロジェクトの目的と使い方 | `README.md` |
| 要件と初回セットアップの指示 | `.prompts/INIT.md` |
| 調査レポート | `docs/reports/` |
| 初回調査: 名前の重複、StreamKit の版、comment-skin からの流用 | `docs/reports/2026-11-10-bootstrap-survey.md` |
| 土台の移植、ローカルのビルドと整形の結果、ポータブル版 OBS の配置 | `docs/reports/2026-11-10-scaffold.md` |
| 前提の検証 V1・V2: StreamKit の実際の DOM、発話中のクラス、`content` による立ち絵の置き換え | `docs/reports/2026-11-10-premise-verification.md` |
| A4-3: 生成した CSS のローカルのページでの表示の確認、名前の重なりの不具合 | `docs/reports/2026-11-10-local-page-check.md` |
| B1: OBS での接続、V1〜V5 の結果と仮決めの確定 | `docs/reports/2026-11-11-runtime-verification.md` |
| 立ち絵の縮小と WebP の埋め込み: 7 枚の大きさと処理時間、既定の画質 | `docs/reports/2026-11-11-portrait-optimization.md` |
| 開発のセッションによる OBS での細かい動作確認 | `docs/reports/2026-11-11-dev-checks.md` |
| 実行ログ (生の出力。git 追跡対象外) | `.logs/` |
| 実行ログ (残す価値があると判断したもの) | `docs/logs/` |
| 計画と決定事項 (Beads 使用時の正本) | `bd show <親課題>` |
| 計画 (Beads 非使用時) | `.prompts/PLANS/` |
| 判断を要する論点 (Beads 非使用時) | `.prompts/DISCUSSIONS/` |
| タスクの状態、依存関係、ブロッカー | `bd ready` / `bd blocked` / `bd list` |
