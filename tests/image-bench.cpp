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

// 立ち絵の縮小と書き出しの大きさと処理時間を計る。プラグインと同じ src/portrait-image.cpp を使う。
// 使い方: speaker-spotlight-image-bench <画像のパスの一覧> [<見た目の確認用の出力先>]
// 一覧は UTF-8 で 1 行 1 枚。出力には画像を「サンプル 1」からの番号だけで示し、パスとファイル名を書かない。
// 出力先を渡すと、EMBED_HEIGHT_LIMIT に縮小した WebP を、WEBP_DEFAULT_QUALITY と 100 で sample-<番号>-q<画質>.webp に書く。
// Qt の imageformats のプラグイン (qwebp、qjpeg) を読めるよう、QT_PLUGIN_PATH に Qt の plugins を渡して実行する

#include "core/portrait-embed.h"
#include "core/portrait.h"
#include "portrait-image.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QImageWriter>
#include <QStringList>

#include <algorithm>
#include <cstdio>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace sspot;

namespace {

constexpr int REPEAT = 3;
constexpr int SCALES[] = {100, 75, 50};

struct Config {
	const char *name;
	core::ImageFormat format;
	int quality;
};

constexpr Config CONFIGS[] = {
	{"webp100", core::ImageFormat::Webp, 100}, {"webp90", core::ImageFormat::Webp, 90},
	{"webp80", core::ImageFormat::Webp, 80},   {"webp75", core::ImageFormat::Webp, 75},
	{"png", core::ImageFormat::Png, -1},
};

double ms(qint64 ns)
{
	return static_cast<double>(ns) / 1e6;
}

template<typename F> double median_ms(F &&run)
{
	std::vector<qint64> times;
	for (int i = 0; i < REPEAT; ++i) {
		QElapsedTimer timer;
		timer.start();
		run();
		times.push_back(timer.nsecsElapsed());
	}
	std::sort(times.begin(), times.end());
	return ms(times[times.size() / 2]);
}

// 透明な画素の色は WebP の可逆でも保たれないため、不透明度が 0 の画素は不透明度だけを比べる
long long differing_pixels(const QImage &expected, const QImage &actual)
{
	if (expected.size() != actual.size())
		return -1;
	const QImage a = expected.convertToFormat(QImage::Format_ARGB32);
	const QImage b = actual.convertToFormat(QImage::Format_ARGB32);
	long long count = 0;
	for (int y = 0; y < a.height(); ++y) {
		const auto *row_a = reinterpret_cast<const QRgb *>(a.constScanLine(y));
		const auto *row_b = reinterpret_cast<const QRgb *>(b.constScanLine(y));
		for (int x = 0; x < a.width(); ++x) {
			const QRgb pa = row_a[x];
			const QRgb pb = row_b[x];
			if (qAlpha(pa) != qAlpha(pb) || (qAlpha(pa) != 0 && (pa & 0xffffffu) != (pb & 0xffffffu)))
				++count;
		}
	}
	return count;
}

QImage decode(const core::EncodedImage &image)
{
	return QImage::fromData(QByteArray(image.bytes.data(), static_cast<qsizetype>(image.bytes.size())));
}

// 計測で求めた段ごとの大きさを返す偽のエンコーダーで、プラグインと同じ段の選び方を模擬する
core::EmbedChoice simulate(const std::vector<std::size_t> &sizes, const std::vector<core::EmbedStep> &steps,
			   core::ImageFormat format)
{
	return core::choose_embedding(steps, [&](const core::EmbedStep &step) -> std::optional<core::EncodedImage> {
		const auto position =
			static_cast<std::size_t>(std::find(steps.begin(), steps.end(), step) - steps.begin());
		return core::EncodedImage{format, std::string(sizes[position], 'x'), 0, 0};
	});
}

} // namespace

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	if (argc < 2) {
		std::fprintf(stderr, "usage: speaker-spotlight-image-bench <list> [<output dir>]\n");
		return 2;
	}
	QFile list(QString::fromLocal8Bit(argv[1]));
	if (!list.open(QIODevice::ReadOnly)) {
		std::fprintf(stderr, "cannot open the list\n");
		return 2;
	}
	QStringList paths;
	for (const QString &line : QString::fromUtf8(list.readAll()).split(QLatin1Char('\n')))
		if (!line.trimmed().isEmpty())
			paths.push_back(line.trimmed());
	const QString output = argc >= 3 ? QString::fromLocal8Bit(argv[2]) : QString();
	if (!output.isEmpty())
		QDir().mkpath(output);

	const core::ImageFormat format = embed_format();
	const std::vector<core::EmbedStep> steps = core::embed_steps(format == core::ImageFormat::Webp);
	std::printf("embed format\t%s\n", std::string(core::mime_type(format)).c_str());
	std::printf("default quality\t%d\n", core::WEBP_DEFAULT_QUALITY);

	std::printf("\n[measure]\nsample\tfile_bytes\twidth\theight\talpha\tload_ms\tlimit\tscale\tout_w\tout_h\t"
		    "scale_ms\tconfig\tbytes\twrite_ms\tlossless_diff\talpha_kept\n");
	std::vector<std::vector<std::size_t>> step_sizes(static_cast<std::size_t>(paths.size()));

	for (qsizetype n = 0; n < paths.size(); ++n) {
		const int sample = static_cast<int>(n) + 1;
		const QFileInfo info(paths[n]);
		QElapsedTimer timer;
		timer.start();
		const LoadedImage loaded = load_source_image(paths[n]);
		const double load_ms = ms(timer.nsecsElapsed());
		if (loaded.problem != core::ImageProblem::None) {
			std::printf("%d\tproblem %d\n", sample, static_cast<int>(loaded.problem));
			continue;
		}
		const QImage &source = loaded.image;

		for (const int scale : SCALES) {
			QImage scaled;
			const double scale_ms = median_ms(
				[&] { scaled = scale_portrait(source, core::EMBED_HEIGHT_LIMIT * scale / 100); });
			for (const Config &config : CONFIGS) {
				std::optional<core::EncodedImage> encoded;
				const double write_ms = median_ms(
					[&] { encoded = write_portrait(scaled, config.format, config.quality); });
				if (!encoded) {
					std::printf("%d\t%s\twrite failed\n", sample, config.name);
					continue;
				}
				const QImage back = decode(*encoded);
				// 可逆のはずの形式だけ、元の画素との違いを数える。-2 は数えていないことを示す
				const long long diff = config.quality == 100 || config.format == core::ImageFormat::Png
							       ? differing_pixels(scaled, back)
							       : -2;
				std::printf(
					"%d\t%lld\t%d\t%d\t%s\t%.1f\t%d\t%.2f\t%d\t%d\t%.1f\t%s\t%zu\t%.1f\t%lld\t%s\n",
					sample, static_cast<long long>(info.size()), source.width(), source.height(),
					source.hasAlphaChannel() ? "yes" : "no", load_ms, core::EMBED_HEIGHT_LIMIT,
					scale / 100.0, scaled.width(), scaled.height(), scale_ms, config.name,
					encoded->bytes.size(), write_ms, diff,
					back.hasAlphaChannel() == scaled.hasAlphaChannel() ? "yes" : "no");
			}
		}

		for (const core::EmbedStep &step : steps) {
			const auto encoded = encode_portrait(
				source, core::EMBED_HEIGHT_LIMIT * step.scale_percent / 100, format, step.quality);
			step_sizes[static_cast<std::size_t>(n)].push_back(encoded ? encoded->bytes.size() : 0);
		}

		if (!output.isEmpty()) {
			// 比べられるよう、可逆の画質 100 も並べて書く
			for (const int quality : {core::WEBP_DEFAULT_QUALITY, 100}) {
				const auto encoded = encode_portrait(source, core::EMBED_HEIGHT_LIMIT,
								     core::ImageFormat::Webp, quality);
				QFile file(QDir(output).filePath(
					QStringLiteral("sample-%1-q%2.webp").arg(sample).arg(quality)));
				if (encoded && file.open(QIODevice::WriteOnly))
					file.write(encoded->bytes.data(), static_cast<qint64>(encoded->bytes.size()));
			}
		}
		std::fflush(stdout);
	}

	std::printf("\n[plan]\nsteps");
	for (const auto &step : steps)
		std::printf("\t%.2fx/q%d", step.scale_percent / 100.0, step.quality);
	std::printf("\n");
	for (std::size_t n = 0; n < step_sizes.size(); ++n) {
		if (step_sizes[n].empty())
			continue;
		const core::EmbedChoice choice = simulate(step_sizes[n], steps, format);
		std::printf("sample %zu\tproblem %d", n + 1, static_cast<int>(choice.problem));
		if (choice.problem == core::EmbedProblem::None)
			std::printf("\tstep %zu\tbytes %zu", choice.step_index, choice.image.bytes.size());
		// プラグインが画像を選んだときと同じ関数で、読み込みから段の選択と書き出しまでの時間を計る
		EmbedResult embedded;
		const double embed_ms =
			median_ms([&] { embedded = embed_portrait_file(paths[static_cast<qsizetype>(n)]); });
		if (embedded.portrait)
			std::printf("\tembed %dx%d\tbase64 %zu\tembed_ms %.1f", embedded.portrait->width,
				    embedded.portrait->height, embedded.portrait->base64.size(), embed_ms);
		std::printf("\n");
	}
	return 0;
}
