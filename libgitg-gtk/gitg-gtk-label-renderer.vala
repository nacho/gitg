namespace GitgGtk
{
	public class LabelRenderer
	{
		private static const int margin = 3;
		private static const int padding = 4;

		private static string label_text(Gitg.Ref r)
		{
			var escaped = Markup.escape_text(r.parsed_name.shortname);
			return "<span size='smaller'>%s</span>".printf(escaped);
		}

		private static int get_label_width(Pango.Layout layout,
		                                   Gitg.Ref     r)
		{
			var smaller = label_text(r);

			int w;

			layout.set_markup(smaller, -1);
			layout.get_pixel_size(out w, null);

			return w + padding * 2;
		}

		public static int width(Gtk.Widget             widget,
		                        Pango.FontDescription *font,
		                        SList<Gitg.Ref>        labels)
		{
			if (labels == null)
			{
				return 0;
			}

			int ret = 0;

			var ctx = widget.get_pango_context();
			var layout = new Pango.Layout(ctx);

			layout.set_font_description(font);

			foreach (Gitg.Ref r in labels)
			{
				ret += get_label_width(layout, r) + margin;
			}

			return ret + margin;
		}

		private static void
		rounded_rectangle(Cairo.Context context,
		                  double        x,
		                  double        y,
		                  double        width,
		                  double        height,
		                  double        radius)
		{
			context.move_to(x + radius, y);
			context.rel_line_to(width - 2 * radius, 0);
			context.arc(x + width - radius, y + radius, radius, 1.5 * Math.PI, 0.0);

			context.rel_line_to(0, height - 2 * radius);
			context.arc(x + width - radius, y + height - radius, radius, 0.0, 0.5 * Math.PI);

			context.rel_line_to(-(width - radius * 2), 0);
			context.arc(x + radius, y + height - radius, radius, 0.5 * Math.PI, Math.PI);

			context.rel_line_to(0, -(height - radius * 2));
			context.arc(x + radius, y + radius, radius, Math.PI, 1.5 * Math.PI);
		}

		private static void get_type_color(Gitg.RefType type,
		                                   out double   r,
		                                   out double   g,
		                                   out double   b)
		{
			switch (type)
			{
				case Gitg.RefType.NONE:
					r = 1;
					g = 1;
					b = 0.8;
				break;
				case Gitg.RefType.BRANCH:
					r = 0.8;
					g = 1;
					b = 0.5;
				break;
				case Gitg.RefType.REMOTE:
					r = 0.5;
					g = 0.8;
					b = 1;
				break;
				case Gitg.RefType.TAG:
					r = 1;
					g = 1;
					b = 0;
				break;
				case Gitg.RefType.STASH:
					r = 1;
					g = 0.8;
					b = 0.5;
				break;
				default:
					r = 1;
					g = 1;
					b = 1;
				break;
			}
		}

		private static void get_ref_color(Gitg.Ref   rref,
		                                  out double r,
		                                  out double g,
		                                  out double b)
		{
			if (rref.working)
			{
				r = 1;
				g = 0.7;
				b = 0;
			}
			else
			{
				get_type_color(rref.parsed_name.rtype, out r, out g, out b);
			}
		}

		private static void set_source_for_ref_type(Cairo.Context context,
		                                            Gitg.Ref      rref,
		                                            bool          use_state)
		{
			if (use_state)
			{
				switch (rref.state)
				{
					case Gitg.RefState.SELECTED:
						context.set_source_rgb(1, 1, 1);
						return;
					case Gitg.RefState.PRELIGHT:
					{
						double r, g, b;

						get_ref_color(rref, out r, out g, out b);
						context.set_source_rgba(r, g, b, 0.3);
						return;
					}
				}
			}

			double r, g, b;
			get_ref_color(rref, out r, out g, out b);

			context.set_source_rgb(r, g, b);
		}

		private static int render_label(Cairo.Context context,
		                                Pango.Layout  layout,
		                                Gitg.Ref      r,
		                                int           x,
		                                int           y,
		                                int           height,
		                                bool          use_state)
		{
			var smaller = label_text(r);

			layout.set_markup(smaller, -1);

			int w;
			int h;

			layout.get_pixel_size(out w, out h);

			rounded_rectangle(context,
			                  x + 0.5,
			                  y + margin + 0.5,
			                  w + padding * 2,
			                  height - margin * 2,
			                  5);

			set_source_for_ref_type(context, r, use_state);
			context.fill_preserve();

			context.set_source_rgb(0, 0, 0);
			context.stroke();

			context.save();

			context.translate(x + padding, y + (height - h) / 2.0 + 0.5);
			Pango.cairo_show_layout(context, layout);

			context.restore();
			return w;
		}

		public static void draw(Gtk.Widget            widget,
		                        Pango.FontDescription font,
		                        Cairo.Context         context,
		                        SList<Gitg.Ref>       labels,
		                        Gdk.Rectangle         area)
		{
			double pos = margin + 0.5;

			context.save();
			context.set_line_width(1.0);

			var ctx = widget.get_pango_context();
			var layout = new Pango.Layout(ctx);

			layout.set_font_description(font);

			foreach (Gitg.Ref r in labels)
			{
				var w = render_label(context,
				                     layout,
				                     r,
				                     (int)pos,
				                     area.y,
				                     area.height,
				                     true);

				pos += w + padding * 2 + margin;
			}

			context.restore();
		}

		public static Gitg.Ref? get_ref_at_pos(Gtk.Widget            widget,
		                                       Pango.FontDescription font,
		                                       SList<Gitg.Ref>       labels,
		                                       int                   x,
		                                       out int               hot_x)
		{
			hot_x = 0;

			if (labels == null)
			{
				return null;
			}

			var ctx = widget.get_pango_context();
			var layout = new Pango.Layout(ctx);

			layout.set_font_description(font);

			int start = margin;
			Gitg.Ref? ret = null;

			foreach (Gitg.Ref r in labels)
			{
				int width = get_label_width(layout, r);

				if (x >= start && x <= start + width)
				{
					ret = r;
					hot_x = x - start;

					break;
				}

				start += width + margin;
			}

			return ret;
		}

		private static uchar convert_color_channel(uchar color,
		                                           uchar alpha)
		{
			return (uchar)((alpha != 0) ? (color / (alpha / 255.0)) : 0);
		}

		private static void convert_bgra_to_rgba(uchar[] src,
		                                         uchar[] dst,
		                                         int     width,
		                                         int     height)
		{
			int i = 0;

			for (int y = 0; y < height; ++y)
			{
				for (int x = 0; x < width; ++x)
				{
					dst[i] = convert_color_channel(src[i + 2], src[i + 3]);
					dst[i + 1] = convert_color_channel(src[i + 1], src[i + 3]);
					dst[i + 2] = convert_color_channel(src[i], src[i + 3]);
					dst[i + 3] = src[i + 3];

					i += 4;
				}
			}
		}

		public static Gdk.Pixbuf render_ref(Gtk.Widget            widget,
		                                    Pango.FontDescription font,
		                                    Gitg.Ref              r,
		                                    int                   height,
		                                    int                   minwidth)
		{
			var ctx = widget.get_pango_context();
			var layout = new Pango.Layout(ctx);

			layout.set_font_description(font);

			int width = int.max(get_label_width(layout, r), minwidth);

			var surface = new Cairo.ImageSurface(Cairo.Format.ARGB32,
			                                     width + 2,
			                                     height + 2);

			var context = new Cairo.Context(surface);
			context.set_line_width(1);

			render_label(context, layout, r, 1, 1, height, false);
			var data = surface.get_data();

			Gdk.Pixbuf ret = new Gdk.Pixbuf(Gdk.Colorspace.RGB,
			                                true,
			                                8,
			                                width + 2,
			                                height + 2);

			var pixdata = ret.get_pixels();
			convert_bgra_to_rgba(data, pixdata, width + 2, height + 2);

			return ret;
		}
	}
}

// vi:ts=4