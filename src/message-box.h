/*
Speaker Spotlight
Copyright (C) 2026 AllegroMoltoV <contact@allegromoltov.jp>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

// 確認と警告のダイアログ。OBS は Qt の翻訳を読み込まないため、Qt の標準ボタンの文言は OBS の言語によらず英語になる。
// ボタンの文言はプラグインの locale から付ける

#include <QString>
#include <QWidget>

namespace sspot {

// 「はい」と「いいえ」で確かめる。「はい」を押したときに true を返す
bool ask_yes_no(QWidget *parent, const QString &title, const QString &message);

// 「OK」だけのボタンで警告を出す
void show_warning(QWidget *parent, const QString &title, const QString &message);

} // namespace sspot
