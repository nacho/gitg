/*
 * This file is part of gitg
 *
 * Copyright (C) 2012 - Ignacio Casal Quinteiro
 *
 * gitg is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gitg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gitg. If not, see <http://www.gnu.org/licenses/>.
 */

using Gitg;
using Gtk;

namespace GitgGtk
{
	public class DashView : ListBox
	{
		private class RepositoryData
		{
			public Repository repository;
			public Grid grid;
		}

		construct
		{
			set_activate_on_single_click(false);

			var recent_manager = RecentManager.get_default();
			var items = recent_manager.get_items();

			foreach (var item in items)
			{
				if (item.has_group("gitg"))
				{
					add_repository(item);
				}
			}
		}

		private void add_repository(RecentInfo info)
		{
			File info_file = File.new_for_uri(info.get_uri());
			File repo_file;

			try
			{
				repo_file = Ggit.Repository.discover(info_file);
			}
			catch
			{
				// TODO: remove from the recent manager
				return;
			}

			Gitg.Repository repo;

			try
			{
				repo = new Gitg.Repository(repo_file, null);
			}
			catch
			{
				return;
			}

			var data = new RepositoryData();
			data.repository = repo;
			data.grid = new Grid();
			data.grid.margin = 12;
			data.grid.set_column_spacing(10);

			File? workdir = repo.get_workdir();
			var label = new Label((workdir != null) ? workdir.get_uri() : repo_file.get_uri());
			label.set_ellipsize (Pango.EllipsizeMode.END);
			data.grid.add(label);

			data.grid.set_data<RepositoryData>("data", data);
			data.grid.show_all();
			add(data.grid);
		}

		public override void child_activated(Widget? child)
		{
			var data = child.get_data<RepositoryData>("data");

			if (data != null)
			{
				stdout.printf("activated: %s\n", data.repository.get_workdir().get_uri());
			}
		}

		public static int main(string[] args)
		{
			Gtk.init(ref args);

			var window = new Window();
			window.set_default_size(300, 300);
			window.add(new DashView());
			window.show_all();

			Gtk.main();

			return 0;
		}
	}
}

// ex:ts=4 noet
