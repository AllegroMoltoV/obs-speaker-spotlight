/*
Voice Spotlight
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

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>

#include "voice-spotlight-dock.h"

#include <QByteArray>
#include <QMainWindow>

#include <util/config-file.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static const char *DOCK_ID = "voice-spotlight-dock";

static vspot::VoiceSpotlightDock *g_dock = nullptr;

static void register_dock()
{
	auto *main_window = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	if (!main_window)
		return;

	g_dock = new vspot::VoiceSpotlightDock();
	g_dock->setObjectName(DOCK_ID);

	if (!obs_frontend_add_dock_by_id(DOCK_ID, obs_module_text("DockTitle"), g_dock)) {
		obs_log(LOG_ERROR, "Failed to add dock");
		delete g_dock;
		g_dock = nullptr;
		return;
	}
	obs_log(LOG_INFO, "Dock registered successfully");

	// OBS restores DockState before this event, then hides newly added docks.
	config_t *user_config = obs_frontend_get_user_config();
	if (user_config) {
		const char *saved_state = config_get_string(user_config, "BasicWindow", "DockState");
		if (saved_state && *saved_state) {
			QByteArray dock_state = QByteArray::fromBase64(QByteArray(saved_state));
			if (main_window->restoreState(dock_state)) {
				obs_log(LOG_INFO, "Restored OBS dock state after registration");
			} else {
				obs_log(LOG_WARNING, "Failed to restore OBS dock state after registration");
			}
		}
	}
}

static void on_frontend_event(enum obs_frontend_event event, void *private_data)
{
	UNUSED_PARAMETER(private_data);

	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		// Create dock after OBS UI is ready
		register_dock();
	} else if (event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP && g_dock) {
		g_dock->clear_sources();
	} else if (event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED && g_dock) {
		g_dock->refresh_sources();
	}
}

extern "C" bool obs_module_load(void)
{
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);

	obs_frontend_add_event_callback(on_frontend_event, nullptr);

	return true;
}

extern "C" void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(on_frontend_event, nullptr);

	// Note: OBS will delete the dock widget automatically
	g_dock = nullptr;

	obs_log(LOG_INFO, "plugin unloaded");
}
