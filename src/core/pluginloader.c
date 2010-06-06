/*
 * uhub - A tiny ADC p2p connection hub
 * Copyright (C) 2007-2010, Jan Vidar Krey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "uhub.h"

#ifdef PLUGIN_SUPPORT
#include "plugin_api/handle.h"

struct uhub_plugin* plugin_open(const char* filename)
{
#ifdef HAVE_DLOPEN
	struct uhub_plugin* plugin = (struct uhub_plugin*) hub_malloc_zero(sizeof(struct uhub_plugin));
	if (!plugin)
	{
		return 0;
	}

	plugin->handle = dlopen(filename, RTLD_LAZY);

	if (!plugin->handle)
	{
		LOG_ERROR("Unable to open plugin %s: %s", filename, dlerror());
		hub_free(plugin);
		return 0;
	}

	return plugin;
#else
	return 0;
#endif
}

void plugin_close(struct uhub_plugin* plugin)
{
#ifdef HAVE_DLOPEN
	dlclose(plugin->handle);
	hub_free(plugin);
#endif
}

void* plugin_lookup_symbol(struct uhub_plugin* plugin, const char* symbol)
{
#ifdef HAVE_DLOPEN
	void* addr = dlsym(plugin->handle, symbol);
	return addr;
#else
	return 0;
#endif
}

struct uhub_plugin_handle* plugin_load(const char* filename, const char* config)
{
	plugin_register_f register_f;
	plugin_unregister_f unregister_f;
	int ret;
	struct uhub_plugin_handle* handle = hub_malloc_zero(sizeof(struct uhub_plugin_handle));
	struct uhub_plugin* plugin = plugin_open(filename);

	if (!plugin)
		return NULL;

	if (!handle)
	{
		plugin_close(plugin);
		return NULL;
	}

	handle->handle = plugin;
	register_f = plugin_lookup_symbol(plugin, "plugin_register");
	unregister_f = plugin_lookup_symbol(plugin, "plugin_unregister");

	if (register_f && unregister_f)
	{
		ret = register_f(handle, config);
		if (ret == 0)
		{
			if (handle->plugin_api_version == PLUGIN_API_VERSION && handle->plugin_funcs_size == sizeof(struct plugin_funcs))
			{
				LOG_INFO("Loaded plugin: %s: \"%s\", version %s.", filename, handle->name, handle->version);
				return handle;
			}
			else
			{
				LOG_ERROR("Unable to load plugin: %s - API version mistmatch", filename);
			}
		}
		else
		{
			LOG_ERROR("Unable to load plugin: %s - Failed to initialize", filename);
		}
	}

	plugin_close(plugin);
	hub_free(handle);
	return NULL;
}


#endif /* PLUGIN_SUPPORT */
