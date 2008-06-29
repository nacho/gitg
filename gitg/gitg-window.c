#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gi18n.h>

#include "sexy-icon-entry.h"
#include "config.h"

#include "gitg-utils.h"
#include "gitg-window.h"
#include "gitg-revision-view.h"
#include "gitg-revision-tree-view.h"

#define GITG_WINDOW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GITG_TYPE_WINDOW, GitgWindowPrivate))

struct _GitgWindowPrivate
{
	GitgRepository *repository;
	
	// Widget placeholders
	GtkTreeView *tree_view;
	GtkStatusbar *statusbar;
	GitgRevisionView *revision_view;
	GitgRevisionTreeView *revision_tree_view;
	GtkWidget *search_popup;
};

static void gitg_window_buildable_iface_init(GtkBuildableIface *iface);

G_DEFINE_TYPE_EXTENDED(GitgWindow, gitg_window, GTK_TYPE_WINDOW, 0,
	G_IMPLEMENT_INTERFACE(GTK_TYPE_BUILDABLE, gitg_window_buildable_iface_init));

static GtkBuildableIface parent_iface = { 0, };
static GtkWindowClass *parent_class = NULL;

static void
gitg_window_finalize(GObject *object)
{
	GitgWindow *self = GITG_WINDOW(object);
	
	G_OBJECT_CLASS(gitg_window_parent_class)->finalize(object);
}

static void
on_selection_changed(GtkTreeSelection *selection, GitgWindow *window)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GitgRevision *revision = NULL;
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 0, &revision, -1);
	
	gitg_revision_view_update(window->priv->revision_view, window->priv->repository, revision);
	gitg_revision_tree_view_update(window->priv->revision_tree_view, window->priv->repository, revision);
}

static void
on_search_icon_pressed(SexyIconEntry *entry, SexyIconEntryPosition icon_pos, int button, GitgWindow *window)
{
	gtk_menu_popup(GTK_MENU(window->priv->search_popup), NULL, NULL, NULL, NULL, button, gtk_get_current_event_time());
}

void
search_column_activate(GtkAction *action, gint column, GtkTreeView *tree_view)
{
	if (!gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)))
		return;

	gtk_tree_view_set_search_column(tree_view, column);
}

void
on_subject_activate(GtkAction *action, GitgWindow *window)
{
	search_column_activate(action, 1, window->priv->tree_view);
}

void
on_author_activate(GtkAction *action, GitgWindow *window)
{
	search_column_activate(action, 2, window->priv->tree_view);
}

void
on_date_activate(GtkAction *action, GitgWindow *window)
{
	search_column_activate(action, 3, window->priv->tree_view);
}

void
on_hash_activate(GtkAction *action, GitgWindow *window)
{
	search_column_activate(action, 4, window->priv->tree_view);
}

static gboolean
search_hash_equal_func(GtkTreeModel *model, gchar const *key, GtkTreeIter *iter)
{
	GitgRevision *rv;
	gtk_tree_model_get(model, iter, 0, &rv, -1);
	
	gchar *sha = gitg_revision_get_sha1(rv);
	
	gboolean ret = strncmp(sha, key, strlen(key)) != 0;
	
	g_free(sha);
	g_object_unref(rv);
	
	return ret;
}

static gboolean
search_equal_func(GtkTreeModel *model, gint column, gchar const *key, GtkTreeIter *iter, gpointer userdata)
{
	if (column == 4)
		return search_hash_equal_func(model, key, iter);

	gchar *cmp;
	gtk_tree_model_get(model, iter, column, &cmp, -1);
	
	gchar *s1 = g_utf8_casefold(key, -1);
	gchar *s2 = g_utf8_casefold(cmp, -1);
	
	gboolean ret = strstr(s2, s1) == NULL;
	
	g_free(s1);
	g_free(s2);

	g_free(cmp);
	
	return ret;
}

static void
focus_search(GtkAccelGroup *group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer userdata)
{
	gtk_widget_grab_focus(GTK_WIDGET(userdata));
}

static void
build_search_entry(GitgWindow *window, GtkBuilder *builder)
{
	GtkWidget *box = GTK_WIDGET(gtk_builder_get_object(builder, "hbox_top"));
	GtkWidget *entry = sexy_icon_entry_new();
	
	GtkImage *image = GTK_IMAGE(gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU));
	sexy_icon_entry_set_icon(SEXY_ICON_ENTRY(entry), SEXY_ICON_ENTRY_PRIMARY, image);
	
	gtk_tree_view_set_search_entry(window->priv->tree_view, GTK_ENTRY(entry));
	gtk_widget_show(entry);
	gtk_box_pack_end(GTK_BOX(box), entry, FALSE, FALSE, 0);
	
	GtkBuilder *b = gtk_builder_new();
	gtk_builder_set_translation_domain(b, GETTEXT_PACKAGE);
	GError *error = NULL;

	if (!gtk_builder_add_from_file(b, GITG_UI_DIR "/gitg-menus.xml", &error))
	{
		g_critical("Could not open UI file: %s (%s)", GITG_UI_DIR "/gitg-menus.xml", error->message);
		g_error_free(error);
		exit(1);
	}
	
	GtkUIManager *manager = GTK_UI_MANAGER(gtk_builder_get_object(b, "uiman"));
	window->priv->search_popup = GTK_WIDGET(g_object_ref(gtk_ui_manager_get_widget(manager, "/ui/search_popup")));
		
	gtk_builder_connect_signals(b, window);
	g_object_unref(b);
	
	g_signal_connect(entry, "icon-pressed", G_CALLBACK(on_search_icon_pressed), window);
	gtk_tree_view_set_search_column(window->priv->tree_view, 1);
	
	gtk_tree_view_set_search_equal_func(window->priv->tree_view, search_equal_func, window, NULL);
	
	GtkAccelGroup *group = gtk_accel_group_new();
	
	GClosure *closure = g_cclosure_new(G_CALLBACK(focus_search), entry, NULL); 
	gtk_accel_group_connect(group, GDK_f, GDK_CONTROL_MASK, 0, closure); 
	gtk_window_add_accel_group(GTK_WINDOW(window), group);
}

static void
on_parent_activated(GitgRevisionView *view, gchar *hash, GitgWindow *window)
{
	GtkTreeIter iter;
	
	if (!gitg_repository_find_by_hash(window->priv->repository, hash, &iter))
		return;
	
	gtk_tree_selection_select_iter(gtk_tree_view_get_selection(window->priv->tree_view), &iter);
	GtkTreePath *path;
	
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(window->priv->repository), &iter);
	
	gtk_tree_view_scroll_to_cell(window->priv->tree_view, path, NULL, FALSE, 0, 0);
	gtk_tree_path_free(path);
}

static void
gitg_window_parser_finished(GtkBuildable *buildable, GtkBuilder *builder)
{
	if (parent_iface.parser_finished)
		parent_iface.parser_finished(buildable, builder);

	// Store widgets
	GitgWindow *window = GITG_WINDOW(buildable);
	window->priv->tree_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "tree_view_rv"));
	window->priv->statusbar = GTK_STATUSBAR(gtk_builder_get_object(builder, "statusbar"));
	window->priv->revision_view = GITG_REVISION_VIEW(gtk_builder_get_object(builder, "revision_view"));
	window->priv->revision_tree_view = GITG_REVISION_TREE_VIEW(gtk_builder_get_object(builder, "revision_tree_view"));
	
	// Create search entry
	build_search_entry(window, builder);
	
	// Connect signals
	GtkTreeSelection *selection = gtk_tree_view_get_selection(window->priv->tree_view);
	g_signal_connect(selection, "changed", G_CALLBACK(on_selection_changed), window);
	g_signal_connect(window->priv->revision_view, "parent-activated", G_CALLBACK(on_parent_activated), window);
}

static void
gitg_window_buildable_iface_init(GtkBuildableIface *iface)
{
	parent_iface = *iface;
	
	iface->parser_finished = gitg_window_parser_finished;
}

static void
gitg_window_destroy(GtkObject *object)
{
	gtk_tree_view_set_model(GITG_WINDOW(object)->priv->tree_view, NULL);

	if (GTK_OBJECT_CLASS(parent_class)->destroy)
		GTK_OBJECT_CLASS(parent_class)->destroy(object);
}

static void
gitg_window_class_init(GitgWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS(klass);
	
	parent_class = g_type_class_peek_parent(klass);
		
	object_class->finalize = gitg_window_finalize;
	gtkobject_class->destroy = gitg_window_destroy;
	
	g_type_class_add_private(object_class, sizeof(GitgWindowPrivate));
}

static void
gitg_window_init(GitgWindow *self)
{
	self->priv = GITG_WINDOW_GET_PRIVATE(self);
}

static void
on_begin_loading(GitgRunner *loader, GitgWindow *window)
{
	GdkCursor *cursor = gdk_cursor_new(GDK_WATCH);
	gdk_window_set_cursor(GTK_WIDGET(window->priv->tree_view)->window, cursor);
	gdk_cursor_unref(cursor);

	gtk_statusbar_push(window->priv->statusbar, 0, _("Begin loading repository"));
}

static void
on_end_loading(GitgRunner *loader, GitgWindow *window)
{
	gchar *msg = g_strdup_printf(_("Loaded %d revisions"), gtk_tree_model_iter_n_children(GTK_TREE_MODEL(window->priv->repository), NULL));

	gtk_statusbar_push(window->priv->statusbar, 0, msg);
	
	g_free(msg);
	gdk_window_set_cursor(GTK_WIDGET(window->priv->tree_view)->window, NULL);
}

static void
on_update(GitgRunner *loader, gchar **revisions, GitgWindow *window)
{
	gchar *msg = g_strdup_printf(_("Loading %d revisions..."), gtk_tree_model_iter_n_children(GTK_TREE_MODEL(window->priv->repository), NULL));

	gtk_statusbar_push(window->priv->statusbar, 0, msg);
	g_free(msg);
}

static void
handle_no_gitdir(GitgWindow *window)
{
	GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Gitg repository could not be found"));
	
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
}

void
gitg_window_load_repository(GitgWindow *window, gchar const *path)
{
	g_return_if_fail(GITG_IS_WINDOW(window));
	
	if (window->priv->repository)
	{
		gtk_tree_view_set_model(window->priv->tree_view, NULL);
		g_object_unref(window->priv->repository);
	}

	window->priv->repository = gitg_repository_new(path);
	
	if (gitg_repository_get_path(window->priv->repository))
	{
		gtk_tree_view_set_model(window->priv->tree_view, GTK_TREE_MODEL(window->priv->repository));
		GitgRunner *loader = gitg_repository_get_loader(window->priv->repository);
	
		g_signal_connect(loader, "begin-loading", G_CALLBACK(on_begin_loading), window);
		g_signal_connect(loader, "end-loading", G_CALLBACK(on_end_loading), window);
		g_signal_connect(loader, "update", G_CALLBACK(on_update), window);
		
		g_object_unref(loader);
		
		gitg_repository_load(window->priv->repository, NULL);
	}
	else
	{
		handle_no_gitdir(window);
	}
}