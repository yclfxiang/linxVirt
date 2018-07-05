/* -*- Mode: C; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
   Copyright (C) 2010-2011 Red Hat, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"
#include <glib/gi18n.h>

#include <sys/stat.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#ifdef USE_SMARTCARD
#include <vreader.h>
#include "smartcard-manager.h"
#endif

#include "glib-compat.h"
#include "spice-widget.h"
#include "spice-gtk-session.h"
#include "spice-audio.h"
#include "spice-common.h"
#include "spice-cmdline.h"
#include "spice-option.h"
#include "usb-device-widget.h"
#include "spice-widget-priv.h"

#include <sys/types.h>     
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <sys/time.h>
#include <sys/select.h>
#include <sys/select.h>
#include <uuid/uuid.h>
#include "cJSON.h"
#include <curl/curl.h>

typedef struct spice_connection spice_connection;
enum {
    STATE_SCROLL_LOCK,
    STATE_CAPS_LOCK,
    STATE_NUM_LOCK,
    STATE_MAX,
};

#define SPICE_TYPE_WINDOW                  (spice_window_get_type ())
#define SPICE_WINDOW(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SPICE_TYPE_WINDOW, SpiceWindow))
#define SPICE_IS_WINDOW(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SPICE_TYPE_WINDOW))
#define SPICE_WINDOW_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), SPICE_TYPE_WINDOW, SpiceWindowClass))
#define SPICE_IS_WINDOW_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), SPICE_TYPE_WINDOW))
#define SPICE_WINDOW_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), SPICE_TYPE_WINDOW, SpiceWindowClass))

typedef struct _SpiceWindow SpiceWindow;
typedef struct _SpiceWindowClass SpiceWindowClass;

struct _SpiceWindow {
    GObject          object;
    spice_connection *conn;
    gint             id;
    gint             monitor_id;
    GtkWidget        *toplevel, *spice;
    GtkWidget        *menubar, *toolbar;

    GtkWidget        *vmbar, *vmlist;
        
    GtkWidget        *ritem, *rmenu;
    GtkWidget        *statusbar, *status, *st[STATE_MAX];
    GtkActionGroup   *ag;
    GtkUIManager     *ui;
    bool             fullscreen;
    bool             mouse_grabbed;
    SpiceChannel     *display_channel;
#ifdef G_OS_WIN32
    gint             win_x;
    gint             win_y;
#endif
    bool             enable_accels_save;
    bool             enable_mnemonics_save;
};

struct _SpiceWindowClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE (SpiceWindow, spice_window, G_TYPE_OBJECT);

#define CHANNELID_MAX 4
#define MONITORID_MAX 4

// FIXME: turn this into an object, get rid of fixed wins array, use
// signals to replace the various callback that iterate over wins array
struct spice_connection {
    SpiceSession     *session;
    SpiceGtkSession  *gtk_session;
    SpiceMainChannel *main;
    SpiceWindow     *wins[CHANNELID_MAX * MONITORID_MAX];
    SpiceAudio       *audio;
    const char       *mouse_state;
    const char       *agent_state;
    gboolean         agent_connected;
    int              channels;
    int              disconnecting;
};

typedef struct conn_info conn_info;
struct conn_info {
    char *vmName;
    spice_connection *conn;
    SpiceChannel *display_channel;
    int id;
    gint monitor_id;
    GtkWidget *spice;
    SpiceChannel *channel;
};

static char current_conn_vm[128];
static char new_vm_imgId[37];
static GSList *conn_list = NULL;
static cJSON *allVmInfo = NULL;
static cJSON *cjson_recv = NULL;
static GString *recv_check;
static pthread_mutex_t lock;
static GtkWidget *dialog_progress, *dialog_spinner;
static GtkWidget *snapshots_dialog, *images_dialog;
/* libcurl */
static GString* url;
static GString *params;
static char cookies[2048];
static cJSON *vmid_list;

static guint timer;
gboolean IS_VM_PAUSE = FALSE;
gboolean IS_LOGIN = FALSE;
gboolean IS_DEL = FALSE;
SpiceChannel *current_channel = NULL;

struct login_win {
    GtkWidget *vbox;
    GtkWidget *hbox1;
    GtkWidget *hbox2;
    GtkWidget *hbox3;
    GtkWidget *hostName;
    GtkWidget *hostName_entry;
    GtkWidget *button_search; 
    GtkWidget *userName;
    GtkWidget *userName_entry;
    GtkWidget *passwd;
    GtkWidget *passwd_entry;
    GtkWidget *sep;
    GtkWidget *message;
    GtkWidget *button_login;
}wgt;
typedef struct snapshot_info snapshot_info;
struct snapshot_info {
    char *name;
    char *description;
    char *imgGUID;
    char *volUUID;
    char *srcVolUUID;
    int active;
    int top;
    GtkWidget *name_entry;
    GtkWidget *description_entry;
    GtkWidget *del_bun;
    GtkWidget *re_bun;
};

static GtkWidget* login_init(void);  
static void global_init(void);
static void global_free(void);

static void search_engine(void);
static void* search_engine_go(void *data);

static void login_engine(GtkButton *bun, gpointer data);
static void quit_login(void);
static int logout(void *data); 

static int update_login_bun_status(gpointer data);
static int update_login_lab_text(gpointer data);
static int update_vmlist(gpointer data);

static int get_allVmInfo(void *data);
static gboolean deal_time(gpointer data);

static void connect_vm(gpointer data);
static int connect_vm_go(void *data);
static void while_conn_shutdown(spice_connection *data);
static GtkWidget* vm_set_diswindow(char *vm, int vm_status);

static void vmbar_switch_page(GtkNotebook *notebook, GtkWidget *page, 
                                           guint page_num, gpointer data);
static conn_info* get_conn_from_list_by_vmname(char *data);
static conn_info* get_conn_from_list_by_conn(spice_connection *data);
static int get_vm_status(char *data);
static int dialog_for_message(void *data);

static char* get_cjson_data(cJSON *cj, const char *key);
static int close_progress_dialog(void *data);

static void vm_list_selection_changed(GtkTreeSelection* selection, gpointer data);
static SpiceWindow* create_spice_window(void);
static SpiceWindow* update_spice_window(conn_info *data);

static int handle_sock_recvInfo(void *data);
static void pause_take_shot(void *data);
static void menu_cb_StartVm(GtkToolButton *toolbutton, void *data);

static spice_connection *connection_new(void);
static void connection_connect(spice_connection *conn);
static void connection_disconnect(spice_connection *conn);
static void connection_destroy(spice_connection *conn);
static void usb_connect_failed(GObject               *object,
                               SpiceUsbDevice        *device,
                               GError                *error,
                               gpointer               data);
static gboolean is_gtk_session_property(const gchar *property);
static void add_window(spice_connection *conn, SpiceWindow *win);
static void del_window(spice_connection *conn, SpiceWindow *win);
static void main_channel_event(SpiceChannel *channel, SpiceChannelEvent event, gpointer data);
static void main_agent_update(SpiceChannel *channel, gpointer data);

/* options */
static gboolean fullscreen = false;
static gboolean version = false;
static char *spicy_title = NULL;
/* globals */
static GMainLoop     *mainloop = NULL;
static int           connections = 0;
static GKeyFile      *keyfile = NULL;
static SpicePortChannel*stdin_port = NULL;

static SpiceWindow *win;

/* ------------------------------------------------------------------ */
static int check_recv_end(char *data)
{
    char ch;  
    int count=0, i=0;            
    while((ch=data[i++]) != '\0')
    {
        if(ch=='{')
            count++; 
        if(ch=='}'&&count==0)
            return 0;
        if(ch=='}'&&count!=0)
            count--;
    }
    if(count==0)
        return 1;
    else
        return 0;
}
static size_t recv_info_end(char *recv, size_t n, size_t l, void *task)
{
    int status; 

    if(NULL == recv)
    {
        cjson_recv = NULL;
        return n*l;
    }
    
    g_print("\nrecv= %s len= %d %d %d\n", recv, (int)strlen(recv), (int)n ,(int)l);

    g_string_append(recv_check, recv);
    
    if(check_recv_end(recv_check->str))
    {
        cjson_recv = cJSON_Parse(recv_check->str);
        if(strstr(task, "get_vm_info"))
        {
            status = cJSON_GetObjectItem(cjson_recv, "status")->valueint;
            if(0 == status)
            {
                char *vmName, *vmId;
                cJSON *vm = cJSON_GetObjectItem(cjson_recv, "vmInfo");
                vmName = get_cjson_data(vm, "vmName");
                vmId = get_cjson_data(vm, "vmId");
                if(NULL == get_cjson_data(vmid_list, vmName))
                    cJSON_AddItemToObject(vmid_list, vmName, cJSON_CreateString(vmId));
                else
                    cJSON_ReplaceItemInObject(vmid_list, vmName, cJSON_CreateString(vmId)); 
            }    
        }
    }
    return n*l;
}
static int save_cookies(CURL *curl, char *dst_cookies)
{
	CURLcode res;
	struct curl_slist *cookies;
	struct curl_slist *nc;
	int i;
    
	res = curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies);
	if(res != CURLE_OK) {
	    fprintf(stderr, "Curl curl_easy_getinfo failed: %s\n",
	    curl_easy_strerror(res));
        return -1;
	}
	nc = cookies;
	i = 1;
	while(nc) {
	    strcpy(dst_cookies,nc->data);
	    nc = nc->next;
	    i++;
	}

	if(i == 1)
        printf("(none)\n");
	
	curl_slist_free_all(cookies);
    return 0;
}
static void* curl_send_get(void *task)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        g_idle_add(dialog_for_message, _("资源繁忙,请稍后再试!"));
        return NULL;
    }

    recv_check = g_string_new(NULL);
    CURL *curl = curl_easy_init();  
    CURLcode res;  
    guint timeout = 120;
    GTimer *time = g_timer_new();

    if (curl)  
    {  
        curl_easy_setopt(curl, CURLOPT_COOKIELIST, cookies);
        curl_easy_setopt(curl, CURLOPT_URL, url->str);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, recv_info_end);  
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, task);  
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1l);  
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10); 
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);  

        printf(" will do  curl_easy_perform(): \n");
        printf("get_req=%s   task=%s\n", url->str, (char*)task);
        g_timer_start(time);
        res = curl_easy_perform(curl); 
        g_timer_stop(time);
        printf("  do  curl_easy_perform() end: \n");

        if (res != CURLE_OK)  
        {  
            g_print("\nSend curl faild: %s    res=%d\n\n", curl_easy_strerror(res), res);
            
            cjson_recv = NULL;
            switch(res)  
            {  
                case CURLE_OPERATION_TIMEDOUT:  
                    g_idle_add(close_progress_dialog, NULL);
                    if((int)g_timer_elapsed(time, NULL) < 20)
                    {
                        if(IS_LOGIN)
                            g_idle_add(logout, NULL);
                        g_idle_add(dialog_for_message, _("服务器失去响应\n无法连接到管理引擎!"));
                    } else {
                        g_idle_add(dialog_for_message, _("执行超时!"));
                        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
                    }
                    break ;
                    
                case CURLE_COULDNT_CONNECT:   
                    g_idle_add(close_progress_dialog, NULL);
                    if(IS_LOGIN)
                        g_idle_add(logout, NULL);
                    g_idle_add(dialog_for_message, _("连接管理引擎失败\n请检查网络和管理引擎状态!"));
                    break ;    
                
                default:  
                    g_idle_add(close_progress_dialog, NULL);
                    g_idle_add(dialog_for_message, _("发送请求失败!"));
                    break ;
            }  
            if(strstr(task, "permit"))
            {
                g_idle_add(update_login_bun_status, NULL);
                g_idle_add(update_login_lab_text, _("欢迎使用凝思虚拟化产品!"));
            }
            g_string_free(url, TRUE);
            pthread_mutex_unlock(&lock);
            g_string_free(recv_check, TRUE);
            curl_easy_cleanup(curl);
            g_timer_destroy(time);
            return NULL;  
        }
        if(strstr(task, "permit"))
            save_cookies(curl, cookies);
    }  

    g_timer_destroy(time);
    if(strstr(task, "get_vm_info") || strstr(task, "get_data_domain") || strstr(task, "get_img_type") ||
         strstr(task, "get_isolist") || strstr(task, "get_unattach_image") || strstr(task, "get_temps")) 
    {
        if(strstr(task, "get_unattach_image"))
            g_idle_add(close_progress_dialog, NULL);
        g_string_free(url, TRUE);
        curl_easy_cleanup(curl);
        g_string_free(recv_check, TRUE);
        pthread_mutex_unlock(&lock);
        return NULL;
    }
    
    g_string_free(url, TRUE);
    g_string_free(recv_check, TRUE);
    curl_easy_cleanup(curl);
    pthread_mutex_unlock(&lock);

    g_idle_add(handle_sock_recvInfo, task);
    return NULL;
}
static void* curl_send_post(void *task)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        g_idle_add(dialog_for_message, _("资源繁忙,请稍后再试!"));
        return NULL;
    }

    recv_check = g_string_new(NULL);
    CURL *curl = curl_easy_init();  
    CURLcode res;
    guint timeout = 120;
    GTimer *time = g_timer_new();

    if(strstr(task, "post_start"))
       timeout = 30;
    if(strstr(task, "post_pause"))
       timeout = 300;
    if (curl)  
    {  
        curl_easy_setopt(curl, CURLOPT_COOKIELIST, cookies);
        curl_easy_setopt(curl, CURLOPT_POST, 1); 
        curl_easy_setopt(curl, CURLOPT_URL, url->str); 
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params->str);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);     
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, recv_info_end);  
	    curl_easy_setopt(curl, CURLOPT_WRITEDATA, task);  
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);  
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);  
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);  

        printf("\n will do  curl_easy_perform(): \n");
        g_timer_start(time);
        printf("post_req=%s  params= (%s)  task=%s\n", url->str, params->str, (char*)task);
        
        res = curl_easy_perform(curl);
        g_timer_stop(time);
        printf("\n  do  curl_easy_perform() end: \n");
        
        if (res != CURLE_OK)  
        {  
            g_print("\n\nSend curl faild: %s    res=%d\n\n", curl_easy_strerror(res), res);
            
            cjson_recv = NULL;
            switch(res)  
            {  
                case CURLE_OPERATION_TIMEDOUT:  
                    g_idle_add(close_progress_dialog, NULL);
                    if((int)g_timer_elapsed(time, NULL) < 20)
                    {
                        if(IS_LOGIN)
                            g_idle_add(logout, NULL);
                        g_idle_add(dialog_for_message, _("服务器失去响应\n无法连接到管理引擎!"));
                    } else {
                        g_idle_add(dialog_for_message, _("执行超时!"));
                        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
                    }
                    break ;
                case CURLE_COULDNT_CONNECT:   
                    g_idle_add(close_progress_dialog, NULL);
                    if(IS_LOGIN)
                        g_idle_add(logout, NULL);
                    g_idle_add(dialog_for_message, _("连接管理引擎失败\n请检查网络和管理引擎状态!"));
                    break ;
                default:  
                    g_idle_add(close_progress_dialog, NULL);
                    g_idle_add(dialog_for_message, _("发送请求失败!"));
                    break ;
            }  
            g_string_free(url, TRUE);
            g_string_free(params, TRUE);
            g_string_free(recv_check, TRUE);
            curl_easy_cleanup(curl);
            pthread_mutex_unlock(&lock);
            return NULL;  
        }
    }  
   
    g_timer_destroy(time);
    if(strstr(task, "post_conn_start") || strstr(task, "post_new_image") 
                                            || strstr(task, "post_check_snap")) 
    {
        if(strstr(task, "post_new_image"))
            g_idle_add(close_progress_dialog, NULL);
        g_string_free(url, TRUE);
        g_string_free(params, TRUE);
        g_string_free(recv_check, TRUE);
        curl_easy_cleanup(curl);
        pthread_mutex_unlock(&lock); 
        return NULL;  
    }
    
    g_string_free(url, TRUE);
    g_string_free(params, TRUE);
    g_string_free(recv_check, TRUE);
    curl_easy_cleanup(curl);
    pthread_mutex_unlock(&lock);

    g_idle_add(handle_sock_recvInfo, task);
    return NULL;
}
static void on_entry_insert_text(GtkEditable *editable,
                                 gchar       *new_text,
                                 gint         new_text_length,
                                 gint        *position,
                                 gpointer     user_data)
{
    if (new_text[0] > '9' || new_text [0] < '0')
    {
       new_text[0] = '\0';
    }
    return ;
}
static void on_mac_entry_insert_text(GtkEditable *editable,
                                 gchar       *new_text,
                                 gint         new_text_length,
                                 gint        *position,
                                 gpointer     user_data)
{
    if(2 == (*position % 3))
    {
       new_text[0] = ':';
    }
}
static int update_login_bun_status(gpointer data)
{   
    gtk_widget_set_sensitive(wgt.button_search, TRUE);
    gtk_widget_set_sensitive(wgt.button_login, TRUE);
    return 0;
}
static int update_login_lab_text(gpointer data)
{
    gtk_label_set_text(GTK_LABEL(wgt.message), data);
    return 0;
}
static void update_toolbar_status(int status)
{
    GtkWidget *toolbar;
    GtkWidget *Fullscreen, *SelectUsbDevices, *VmRunOnce, *ChangeCd, *VmStart, *VmPause, 
        *VmStop, *VmRestart, *SaveSnapShot, *ManageSnapShot, *CreateVm, *EditVm, *DeleteVm, *ManageImage;

    toolbar = win->toolbar;
    Fullscreen = (GtkWidget *)gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 0);
    SelectUsbDevices = (GtkWidget *)gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 2);
    CreateVm = (GtkWidget *)gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 4);
    EditVm = (GtkWidget *)gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 5);
    DeleteVm = (GtkWidget *)gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 6);
    VmRunOnce = (GtkWidget *)gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 8);
    ChangeCd = (GtkWidget *)gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 9);
    VmStart = (GtkWidget *)gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 11);
    VmPause = (GtkWidget *)gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 12);
    VmStop = (GtkWidget *)gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 13);
    VmRestart = (GtkWidget *)gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 14);
    SaveSnapShot = (GtkWidget *)gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 16);
    ManageSnapShot = (GtkWidget *)gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 17);
    ManageImage = (GtkWidget *)gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 19);
    
printf("update_toolbar_status(): status= %d\n", status);

    if(!gtk_widget_get_sensitive(toolbar))
        gtk_widget_set_sensitive(toolbar, TRUE);
    
    switch( status )
    {
        case -1:
            if(IS_LOGIN)
            {
                gtk_widget_set_sensitive(CreateVm, TRUE);
                gtk_widget_set_sensitive(EditVm, FALSE);
                gtk_widget_set_sensitive(DeleteVm, FALSE);
                gtk_widget_set_sensitive(Fullscreen, TRUE);
                gtk_widget_set_sensitive(SelectUsbDevices, FALSE);
                gtk_widget_set_sensitive(VmRunOnce, FALSE);
                gtk_widget_set_sensitive(ChangeCd, FALSE);
                gtk_widget_set_sensitive(VmStart, FALSE);
                gtk_widget_set_sensitive(VmPause, FALSE);
                gtk_widget_set_sensitive(VmStop, FALSE);
                gtk_widget_set_sensitive(VmRestart, FALSE);
                gtk_widget_set_sensitive(SaveSnapShot, FALSE);
                gtk_widget_set_sensitive(ManageSnapShot, FALSE);
                gtk_widget_set_sensitive(ManageImage, TRUE);
            }
            else
                gtk_widget_set_sensitive(toolbar, FALSE);
            break;
        case 1:
            gtk_widget_set_sensitive(SelectUsbDevices, TRUE);
            gtk_widget_set_sensitive(VmRunOnce, FALSE);
            gtk_widget_set_sensitive(ChangeCd, TRUE);
            gtk_widget_set_sensitive(VmStart, FALSE);
            gtk_widget_set_sensitive(VmPause, TRUE);
            gtk_widget_set_sensitive(VmStop, TRUE);
            gtk_widget_set_sensitive(VmRestart, TRUE);
            gtk_widget_set_sensitive(SaveSnapShot, TRUE);
            gtk_widget_set_sensitive(ManageSnapShot, TRUE);
            gtk_widget_set_sensitive(EditVm, TRUE);
            gtk_widget_set_sensitive(DeleteVm, FALSE);
            gtk_widget_set_sensitive(ManageImage, TRUE);
            break;
        case 2:
            gtk_widget_set_sensitive(SelectUsbDevices, TRUE);
            gtk_widget_set_sensitive(VmRunOnce, FALSE);
            gtk_widget_set_sensitive(ChangeCd, TRUE);
            gtk_widget_set_sensitive(VmStart, FALSE);
            gtk_widget_set_sensitive(VmPause, TRUE);
            gtk_widget_set_sensitive(VmStop, TRUE);
            gtk_widget_set_sensitive(VmRestart, TRUE);
            gtk_widget_set_sensitive(SaveSnapShot, TRUE);
            gtk_widget_set_sensitive(ManageSnapShot, TRUE);
            gtk_widget_set_sensitive(DeleteVm, FALSE);
            gtk_widget_set_sensitive(ManageImage, TRUE);
            break;
        case 3:
            gtk_widget_set_sensitive(SelectUsbDevices, FALSE);
            gtk_widget_set_sensitive(VmRunOnce, FALSE);
            gtk_widget_set_sensitive(ChangeCd, FALSE);
            gtk_widget_set_sensitive(VmStart, TRUE);
            gtk_widget_set_sensitive(VmPause, FALSE);
            gtk_widget_set_sensitive(VmStop, TRUE);
            gtk_widget_set_sensitive(VmRestart, FALSE);
            gtk_widget_set_sensitive(SaveSnapShot, FALSE);
            gtk_widget_set_sensitive(ManageSnapShot, TRUE);
            gtk_widget_set_sensitive(EditVm, TRUE);
            gtk_widget_set_sensitive(DeleteVm, FALSE);
            gtk_widget_set_sensitive(ManageImage, TRUE);
            break;
        case 100:    
            gtk_widget_set_sensitive(CreateVm, TRUE);
            gtk_widget_set_sensitive(EditVm, FALSE);
            gtk_widget_set_sensitive(DeleteVm, FALSE);
            gtk_widget_set_sensitive(Fullscreen, TRUE);
            gtk_widget_set_sensitive(SelectUsbDevices, FALSE);
            gtk_widget_set_sensitive(VmRunOnce, FALSE);
            gtk_widget_set_sensitive(ChangeCd, FALSE);
            gtk_widget_set_sensitive(VmStart, FALSE);
            gtk_widget_set_sensitive(VmPause, FALSE);
            gtk_widget_set_sensitive(VmStop, FALSE);
            gtk_widget_set_sensitive(VmRestart, FALSE);
            gtk_widget_set_sensitive(SaveSnapShot, FALSE);
            gtk_widget_set_sensitive(ManageSnapShot, FALSE);
            gtk_widget_set_sensitive(ManageImage, TRUE);
            break ;
        default:
            gtk_widget_set_sensitive(CreateVm, TRUE);
            gtk_widget_set_sensitive(SelectUsbDevices, FALSE);
            gtk_widget_set_sensitive(VmRunOnce, TRUE);
            gtk_widget_set_sensitive(ChangeCd, FALSE);
            gtk_widget_set_sensitive(VmStart, TRUE);
            gtk_widget_set_sensitive(VmPause, FALSE);
            gtk_widget_set_sensitive(VmStop, FALSE);
            gtk_widget_set_sensitive(VmRestart, FALSE);
            gtk_widget_set_sensitive(SaveSnapShot, FALSE);
            gtk_widget_set_sensitive(ManageSnapShot, TRUE);
            gtk_widget_set_sensitive(EditVm, TRUE);
            gtk_widget_set_sensitive(DeleteVm, TRUE);
            gtk_widget_set_sensitive(ManageImage, TRUE);
            break;
    }
}
static char* get_cjson_data(cJSON *cj, const char *key)
{
    char *value;
    
    if(NULL == cJSON_GetObjectItem(cj, key))
        return NULL;
        
    value = cJSON_GetObjectItem(cj, key)->valuestring;
    return value;
}
static char* get_vmbar_vmname(int index)
{
    GtkWidget *page, *event_box, *event_hbox, *label;
    const char *name;
    GList *list;

    page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(win->vmbar), index);
    event_box = gtk_notebook_get_tab_label(GTK_NOTEBOOK(win->vmbar), page);
    event_hbox = gtk_bin_get_child(GTK_BIN(event_box));
    list = gtk_container_get_children(GTK_CONTAINER(event_hbox));
    label = g_list_nth(list, 1)->data;
    name = gtk_label_get_label(GTK_LABEL(label)); 

    g_list_free(list);
    return (char*)name;
}
static int get_vmbar_index(char *data)
{
    const char *vm=data, *name;
    int i, len;
    len = gtk_notebook_get_n_pages(GTK_NOTEBOOK(win->vmbar));

    for(i=2; i<len; i++)
    {
        name = get_vmbar_vmname(i); 
        if(0 == g_strcmp0(vm, name))
        {
            return i;
        }
    }
    return -1;
}
static int get_vmlist_index(char *data)
{
    GtkTreeModel *model;
    GtkTreeIter iter, parent;
    int index=-1, len;
    char *name, *vm=data;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(win->vmlist));
    if(FALSE == gtk_tree_model_get_iter_first(model, &parent))
        return -1;
    do{
        index ++;
        if(gtk_tree_model_iter_has_child(model, &parent))
        {
            len = gtk_tree_model_iter_n_children(model, &parent);
            for(int i=0; i<len; i++)
            {
                gtk_tree_model_iter_nth_child(model, &iter, &parent, i);
                gtk_tree_model_get(model, &iter, 1, &name, -1);
                if(0 == g_strcmp0(vm, name))
                {
                    g_free(name);
                    return index;
                }
                g_free(name);
            }
        }    
    }while(gtk_tree_model_iter_next(model, &parent));
    
    return -1;
}
static void update_vmlist_status(char *data, int status)
{
    GtkTreeModel *model;
    GtkTreeStore *store;
    GtkTreeIter iter, parent;
    int index=-1, len=0;
    char *name=NULL, *vm=data;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(win->vmlist));
    store = GTK_TREE_STORE(model);
    if(FALSE == gtk_tree_model_get_iter_first(model, &parent))
        return ;
    do{
        index ++;
        if(gtk_tree_model_iter_has_child(model, &parent))
        {
            len = gtk_tree_model_iter_n_children(model, &parent);
            for(int i=0; i<len; i++)
            {
                gtk_tree_model_iter_nth_child(model, &iter, &parent, i);
                gtk_tree_model_get(model, &iter, 1, &name, -1);
                if(0 == g_strcmp0(vm, name))
                {
                    switch(status)
                    {
                        case 1:
                            gtk_tree_store_set(store, &iter, 0, GTK_STOCK_MEDIA_PLAY, -1);
                            break;
                        case 2:
                            gtk_tree_store_set(store, &iter, 0, GTK_STOCK_REFRESH, -1);
                            break;
                        case 3:
                            gtk_tree_store_set(store, &iter, 0, GTK_STOCK_MEDIA_PAUSE, -1);
                            break;
                        default:
                            gtk_tree_store_set(store, &iter, 0, GTK_STOCK_MEDIA_STOP, -1);
                            break;     
                    }
                }
                g_free(name);
            }
        }    
    }while(gtk_tree_model_iter_next(model, &parent));
    
    return ;
}
static void delete_vm_for_vmlist(char *vm)
{
    GtkTreeModel *model;
    GtkTreeStore *store;
    GtkTreeIter iter, parent;
    int len=0;
    char *name=NULL, *vmNmae=vm;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(win->vmlist));
    store = GTK_TREE_STORE(model);
    if(FALSE == gtk_tree_model_get_iter_first(model, &parent))
        return ;
    do{
        if(gtk_tree_model_iter_has_child(model, &parent))
        {
            len = gtk_tree_model_iter_n_children(model, &parent);
            for(int i=0; i<len; i++)
            {
                gtk_tree_model_iter_nth_child(model, &iter, &parent, i);
                gtk_tree_model_get(model, &iter, 1, &name, -1);
                if(0 == g_strcmp0(vmNmae, name))
                {
                    gtk_tree_store_remove(store, &iter);
                    g_free(name);
                    return ;
                }
                g_free(name);
            }
        }    
    }while(gtk_tree_model_iter_next(model, &parent));
    
    return ;
}
static conn_info* get_conn_from_list_by_vmname(char *data)
{   
    const char *vm = data;
    conn_info *ci;
    GSList *list = conn_list;
    while(list != NULL)
    {
        ci = list->data;
        if(0 == g_strcmp0(vm, ci->vmName))
            return ci;
        list = g_slist_next(list);
    }
    return NULL;
}
static conn_info* get_conn_from_list_by_conn(spice_connection *data)
{   
    spice_connection *conn = data;
    conn_info *ci;
    GSList *list = conn_list;
    while(list != NULL)
    {
        ci = list->data;
        if(conn == ci->conn)
            return ci;
        list = g_slist_next(list);
    }
    return NULL;
}
static int get_vm_status(char *data)
{
    char *vm=data, *vmName;
    int status;
    cJSON *vms=cJSON_GetObjectItem(allVmInfo, "rows"), *tmp;

    if(NULL == vms)
        return -1;

    for(int i=0; i<cJSON_GetArraySize(vms); i++)
    {   
        tmp = cJSON_GetArrayItem(vms, i);
        vmName = get_cjson_data(tmp, "vmName");
        if(0 == g_strcmp0(vm, vmName))
        {
            status = cJSON_GetObjectItem(tmp, "status")->valueint;
            return status;
        } 
    }
    return -1;
}
static char* get_vm_property(char *name, const char *property)
{
    char *vm=name, *vmName, *value;
    cJSON *vms=cJSON_GetObjectItem(allVmInfo, "rows"), *tmp;
    
    if(NULL == vms)
        return NULL;

    for(int i=0; i<cJSON_GetArraySize(vms); i++)
    {   
        tmp = cJSON_GetArrayItem(vms, i);
        vmName = get_cjson_data(tmp, "vmName");
        if(0 == g_strcmp0(vm, vmName))
        {
            value = get_cjson_data(tmp, property);
            return value;
        }   
    }
    return NULL;
}
static void* get_vm_info(void *data)
{
    char *host, *vmName=data;
    
    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry)); 

    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/vminfo?");
    g_string_append(url, "vmName=");
    g_string_append(url, vmName);
    
    pthread_mutex_unlock(&lock);
    curl_send_get("get_vm_info");

    g_free(host);
    return NULL;
}

static void on_close_page_clicked(GtkButton *button, gpointer data)
{
    GtkWidget *event_box, *event_hbox, *label;
    GList *list;
    const char *vmName;
    int index;
    conn_info *ci;
    SpiceChannel *channel;

    event_box = data;
    event_hbox = gtk_bin_get_child(GTK_BIN(event_box));
    list = gtk_container_get_children(GTK_CONTAINER(event_hbox));
    label = g_list_nth(list, 1)->data;
    vmName = gtk_label_get_label(GTK_LABEL(label)); 
    index = get_vmbar_index((char*)vmName);

    ci = get_conn_from_list_by_vmname((char*)vmName);
    if(NULL != ci)
    {
        channel = ci->channel;
        g_signal_handlers_block_by_func(channel, G_CALLBACK(main_channel_event), ci->conn);
        g_signal_handlers_block_by_func(channel, G_CALLBACK(main_agent_update), ci->conn);
        connection_disconnect(ci->conn);
        conn_list = g_slist_remove(conn_list, ci);
        g_free(ci->vmName);
        g_free(ci);
    } 
    gtk_notebook_remove_page(GTK_NOTEBOOK(win->vmbar), index);
    g_list_free(list);
}
static GtkWidget* linx_noetbook_tab_label(char *data)
{
    GtkWidget *event_box;
    GtkWidget *event_hbox;
    GtkWidget *image, *label, *img_bun;
    GtkToolItem *bun;

	event_box = gtk_event_box_new();
	event_hbox = gtk_hbox_new(FALSE, 0);
    image = gtk_image_new_from_file(_("/etc/linx/ppms/png/computer.png"));

    GdkPixbuf *src = gdk_pixbuf_new_from_file("/etc/linx/ppms/png/close.png", NULL);
    GdkPixbuf *des = gdk_pixbuf_scale_simple(src, 12, 12, GDK_INTERP_BILINEAR);
    img_bun = gtk_image_new_from_pixbuf(des);
        
    label = gtk_label_new(data);
    gtk_label_set_max_width_chars(GTK_LABEL(label), 10);
    bun = gtk_tool_button_new(img_bun, NULL);
    gtk_widget_set_tooltip_text (GTK_WIDGET(bun), _("关闭当前页"));
    gtk_widget_show_all(GTK_WIDGET(bun));
    
	gtk_box_pack_start(GTK_BOX(event_hbox), image, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(event_hbox), label, FALSE, FALSE, 6);
	gtk_box_pack_start (GTK_BOX (event_hbox), GTK_WIDGET(bun), FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(event_box), event_hbox);
    gtk_widget_show_all(event_box);

    g_signal_connect(bun, "clicked", G_CALLBACK(on_close_page_clicked), event_box);

    g_object_unref(src);
    g_object_unref(des);
    return event_box;
}
static GtkWidget* new_image_bun(const gchar *stock_id, char *data)
{
    GtkWidget *bun;
    GtkWidget *hbox;
    GtkWidget *image;
    GtkWidget *label;

    bun = gtk_button_new();
    hbox = gtk_hbox_new(FALSE, 0);
    image = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_MENU);
    label = gtk_label_new(data);
    gtk_container_add(GTK_CONTAINER(bun), hbox);
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 3);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 3);

    return bun;
}

static void global_init(void)
{
    memset(current_conn_vm, 0, sizeof(current_conn_vm));
    conn_list = NULL;
    allVmInfo = NULL;
    cjson_recv = NULL;
    vmid_list = cJSON_CreateObject();

    pthread_mutex_init(&lock, NULL);
}
static void global_free(void)
{
    g_source_remove(timer);
    if(conn_list)
        g_slist_free(conn_list);
    if(allVmInfo)
        cJSON_Delete(allVmInfo);
    if(allVmInfo)
        cJSON_Delete(vmid_list);

    IS_LOGIN = FALSE;
    IS_VM_PAUSE = FALSE;
    pthread_mutex_destroy(&lock);
}
static gboolean deal_time(gpointer data)
{
    pthread_mutex_unlock(&lock);
    
    if(!IS_LOGIN)
        return FALSE;    
    
    get_allVmInfo(NULL);
    return TRUE;
}
static gboolean dialog_for_confirm(const char *task, int *flag)
{
    GtkWidget *dialog, *area, *bun, *bun1;
    gboolean result = FALSE;
    
    dialog = gtk_message_dialog_new(GTK_WINDOW(win->toplevel),
                                    GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, 
                                    GTK_BUTTONS_YES_NO, task, NULL);
    gtk_window_set_title(GTK_WINDOW(dialog), _("请确认:"));
    area = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog));  
    
    bun = gtk_check_button_new_with_label(_("删除虚拟机磁盘"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bun), TRUE);
    bun1 = gtk_check_button_new_with_label(_("擦试删除磁盘"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bun1), FALSE);
    if(strstr(task, "删除虚拟机"))
        gtk_box_pack_start(GTK_BOX(area), bun, TRUE, TRUE, 10);
    if(strstr(task, "删除磁盘"))
        gtk_box_pack_start(GTK_BOX(area), bun1, TRUE, TRUE, 10);
    

    gtk_widget_show_all(dialog);
    switch(gtk_dialog_run(GTK_DIALOG(dialog)))
    {
        case GTK_RESPONSE_YES:
            if(strstr(task, "删除虚拟机"))
            {
                if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(bun)))
                    *flag = 1;
            }
            if(strstr(task, "删除磁盘"))
            {
                if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(bun1)))
                    *flag = 1;
            }
            result = TRUE; 
            break ;
        default:
            result = FALSE;
            break ;
    }
    gtk_widget_destroy(dialog);
    return result;
}
static int dialog_for_message(void *data)
{
    GtkWidget *dialog;

    if(strstr((char*)data, "登录异常"))
        g_source_remove(timer);
    
    dialog = gtk_message_dialog_new(GTK_WINDOW(win->toplevel),
                                    GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, 
                                    GTK_BUTTONS_OK, data, NULL);
    gtk_window_set_title(GTK_WINDOW(dialog),_("执行结果:"));
    gtk_widget_set_size_request(dialog, 250, 150);
    gtk_dialog_run(GTK_DIALOG(dialog)); 
    gtk_widget_destroy(dialog);

    if(strstr((char*)data, "登录异常"))
        logout(NULL);
    return 0;
}
static int dialog_for_progress(const char *data)
{
    GtkWidget *area, *area1;

    dialog_progress = gtk_dialog_new_with_buttons(data, GTK_WINDOW(win->toplevel),
                                     GTK_DIALOG_MODAL,
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     NULL);
    gtk_widget_set_size_request(dialog_progress, 220, 120);
    dialog_spinner = gtk_spinner_new();
   
    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog_progress));
    area1 = gtk_dialog_get_action_area(GTK_DIALOG(dialog_progress));
    gtk_box_pack_start(GTK_BOX(area), dialog_spinner, TRUE, TRUE, 37);
    gtk_window_set_title(GTK_WINDOW(dialog_progress), data);

    gtk_widget_show_all(dialog_progress);
    gtk_widget_hide(area1);
    gtk_spinner_start(GTK_SPINNER(dialog_spinner));
    gtk_dialog_run(GTK_DIALOG(dialog_progress));

    if(GTK_IS_WIDGET(dialog_progress))
        gtk_widget_destroy(dialog_progress);

    return 0;
}
static int close_progress_dialog(void *data)
{
    if(GTK_IS_SPINNER(dialog_spinner))
        gtk_spinner_stop(GTK_SPINNER(dialog_spinner));
    if(GTK_IS_WIDGET(dialog_progress))
        gtk_widget_destroy(dialog_progress);
    return 0;
}

static GtkWidget* init_vmbar(void)
{   
    GtkWidget *vmbar = gtk_notebook_new();
    GtkWidget *login = login_init();
    
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(vmbar), TRUE);
    gtk_notebook_append_page(GTK_NOTEBOOK(vmbar), login, gtk_label_new("HOME"));

    return vmbar;
}

static GtkWidget* login_init(void)
{   
    GtkWidget* login = gtk_frame_new(NULL);
    gtk_container_set_border_width(GTK_CONTAINER(login), 100);
    wgt.vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(login), wgt.vbox);
    gtk_container_set_border_width(GTK_CONTAINER(wgt.vbox), 100);

    wgt.hbox1 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(wgt.vbox), wgt.hbox1, FALSE, FALSE, 5);
    wgt.hostName = gtk_label_new("管理引擎(IP):");
    gtk_box_pack_start(GTK_BOX(wgt.hbox1), wgt.hostName, FALSE, FALSE, 5);
    wgt.hostName_entry = gtk_combo_box_text_new_with_entry();
    gtk_box_pack_start(GTK_BOX(wgt.hbox1), wgt.hostName_entry, TRUE, TRUE, 0);
    wgt.button_search = gtk_button_new_with_label("搜索");
    g_signal_connect(wgt.button_search, "clicked", G_CALLBACK(search_engine), NULL);
    gtk_box_pack_end(GTK_BOX(wgt.hbox1), wgt.button_search, FALSE, FALSE, 0);
    
    wgt.hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(wgt.vbox), wgt.hbox2, FALSE, FALSE, 5);
    wgt.userName = gtk_label_new("用      户       名:");
    gtk_box_pack_start(GTK_BOX(wgt.hbox2), wgt.userName, FALSE, FALSE, 5);
    wgt.userName_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(wgt.hbox2), wgt.userName_entry, TRUE, TRUE, 0);

    wgt.hbox3 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(wgt.vbox), wgt.hbox3, FALSE, FALSE, 5);
    wgt.passwd = gtk_label_new("用  户   密   码:");
    gtk_box_pack_start(GTK_BOX(wgt.hbox3), wgt.passwd, FALSE, FALSE, 5);
    wgt.passwd_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(wgt.passwd_entry), FALSE);
    gtk_box_pack_start(GTK_BOX(wgt.hbox3), wgt.passwd_entry, TRUE, TRUE, 0);

    wgt.sep = gtk_hseparator_new();        
    gtk_box_pack_start(GTK_BOX(wgt.vbox), wgt.sep, FALSE, FALSE, 10);
    wgt.message = gtk_label_new("欢迎使用凝思虚拟化产品!");
    gtk_box_pack_start(GTK_BOX(wgt.vbox), wgt.message, FALSE, FALSE, 30);
    wgt.button_login = gtk_button_new_with_label("登录");
    g_signal_connect(wgt.button_login, "clicked", G_CALLBACK(login_engine), NULL);
    gtk_box_pack_end(GTK_BOX(wgt.vbox), wgt.button_login, FALSE, FALSE, 40);
    
    return login;
}

static void search_engine(void)
{     
    g_idle_add(update_login_lab_text, _("搜索中......"));
    //gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    gtk_widget_set_sensitive(wgt.button_search, FALSE);
    gtk_widget_set_sensitive(wgt.button_login, FALSE);
    
    g_thread_new("search_engine", search_engine_go, NULL);

    return ;
}
static int add_action_engine(gpointer data)
{
    GSList *list = (GSList *)data;
    GSList *list1 = list;
    while(list != NULL)
    {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry), (char *)list->data);
        free(list->data);
        list = g_slist_next(list);
    }
    g_slist_free(list1);
    return 0;
}
static void* search_engine_go(void *data)
{
    int sock = -1;
    fd_set rfd;
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    const int opt = 1;  
    int ret = 0;
    char buff[5] = {"linx"};
    GSList *list = NULL;
    char *ip;
    
    struct sockaddr_in addr;  
    addr.sin_family=AF_INET;  
    addr.sin_addr.s_addr=htonl(INADDR_BROADCAST);  
    addr.sin_port=htons(23344);  
    int len=sizeof(addr);
    
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        g_idle_add(update_login_bun_status, NULL);
        g_idle_add(update_login_lab_text, _("搜索失败!"));
        perror("socket():");  
        return 0;
    }
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt)) < 0)
    {
        g_idle_add(update_login_bun_status, NULL);
        g_idle_add(update_login_lab_text, _("搜索失败!"));
        close(sock);
        perror("setsockopt():");  
        return 0;
    }
    if((sendto(sock, buff, 5, 0, (struct sockaddr *)&addr, len)) < 0)
    {
        g_idle_add(update_login_bun_status, NULL);
        g_idle_add(update_login_lab_text, _("搜索失败!"));
        close(sock);
        perror("sendto():");  
        return 0;
    }
    while( TRUE )
    {
        FD_ZERO(&rfd);
		FD_SET(sock, &rfd);

        if((ret = select(sock+1, &rfd, NULL, NULL, &timeout)) < 0)
        {   
            perror("select():");
            break ;
        } else if ( 0 == ret ) { 
            
            break ;
        } else {
            
            if (FD_ISSET(sock, &rfd))
            {
                ret = recvfrom(sock, buff, 5, 0, (struct sockaddr*)&addr, (socklen_t*)&len);
                if( ret < 0 )
                {
                    perror("recvfrom():");  
                    break ;
                } else {
                    ip = NULL;
                    ip = (char *)malloc(strlen(inet_ntoa(addr.sin_addr))+1);
                    strcpy(ip, inet_ntoa(addr.sin_addr));
                    list = g_slist_append(list, ip);
                }
            }
        }
    }
    
    g_idle_add(add_action_engine, list);
    g_idle_add(update_login_bun_status, NULL);
    g_idle_add(update_login_lab_text, _("搜索完成!"));
    close(sock);
    return 0;
}

static void login_engine(GtkButton *bun, gpointer data)
{   
    char *host;
    const char *user, *passwd;
    
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return ;
    }
    
    global_init();
    
    gtk_widget_set_sensitive(wgt.button_login, FALSE);
    gtk_widget_set_sensitive(wgt.button_search, FALSE);
    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    user = gtk_entry_get_text(GTK_ENTRY(wgt.userName_entry));
    passwd = gtk_entry_get_text(GTK_ENTRY(wgt.passwd_entry));  
    if(0 == g_strcmp0(host,"") || 0 == g_strcmp0(user,"") || 0 == g_strcmp0(passwd,""))
    {
        g_idle_add(update_login_bun_status, NULL);
        g_idle_add(update_login_lab_text, _("输入项不能为空，请检查!"));
        pthread_mutex_unlock(&lock);
        g_free(host);
        return ;
    }
    g_idle_add(update_login_lab_text, _("登录中......"));

    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/usr/loginpermit");

    pthread_mutex_unlock(&lock);
    g_thread_new("get_permit", curl_send_get, "get_permit");  

    g_free(host);
    return ;
}
static void login_engine_1(void *data)
{
    char *host;
    const char *user, *passwd;;
    cJSON *recvInfo=data;
    
    if(NULL == recvInfo)
    {
        dialog_for_message("返回信息异常，解析失败!");
        return ;
    }

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    user = gtk_entry_get_text(GTK_ENTRY(wgt.userName_entry));
    passwd = gtk_entry_get_text(GTK_ENTRY(wgt.passwd_entry));  
    
    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/usr/login");
    params = g_string_new("username=");
    g_string_append(params, user);
    g_string_append(params, "&password=");
    g_string_append(params, passwd);
    
    pthread_mutex_unlock(&lock);
    g_thread_new("post_login", curl_send_post, "post_login");
    
    cJSON_Delete(recvInfo);
    g_free(host);
}

static int login_success(void)
{
    GtkWidget *login, *window, *sep, *vbox, *vbox1, *hbox1, *hbox2, *button;
    GtkWidget *text, *host1, *user1, *host2, *user2;
    char *host;
    const char *user;

    update_login_lab_text(_("欢迎使用凝思虚拟化产品!"));
    update_login_bun_status(NULL);

    host1 = gtk_label_new("登录引擎：");
    user1 = gtk_label_new("登录用户：");
    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    user = gtk_entry_get_text(GTK_ENTRY(wgt.userName_entry));
    host2 = gtk_label_new(host);
    user2 = gtk_label_new(user);
    text = gtk_label_new("您已成功登录：");
    sep = gtk_hseparator_new();  
    button = gtk_button_new_with_label("登出");

    window = gtk_frame_new(NULL);
    //window = gtk_event_box_new();

    vbox = gtk_vbox_new(FALSE, 5);
    vbox1 = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(window), 100);
    gtk_container_set_border_width(GTK_CONTAINER(vbox1), 120);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_box_pack_start(GTK_BOX(vbox), text, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), vbox1, FALSE, FALSE, 5);

    hbox1 = gtk_hbox_new(FALSE, 5);
    hbox2 = gtk_hbox_new(FALSE, 3);
    gtk_box_pack_start(GTK_BOX(hbox1), host1, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox1), host2, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox2), user1, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox2), user2, FALSE, FALSE, 5);

    gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 30);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox2, FALSE, FALSE, 30);
    gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 30);
    
    g_signal_connect(button, "clicked", G_CALLBACK(quit_login), NULL);

    gtk_widget_show_all(window);
    gtk_notebook_insert_page(GTK_NOTEBOOK(win->vmbar), window, gtk_label_new("HOME"), 1);
    login = gtk_notebook_get_nth_page(GTK_NOTEBOOK(win->vmbar), 0);
    gtk_widget_hide(login);
    
    pthread_mutex_unlock(&lock);
    get_allVmInfo(NULL);
    IS_LOGIN = TRUE;
    update_toolbar_status(100);
    timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);

   // int w,h, a, s;
    //gtk_widget_set_size_request(window, &w, &h);

   
    //gtk_widget_get_preferred_width(window, a, s);
    
    //gdk_drawable_get_size(GDK_DRAWABLE(vbox->window), &a, &s);
    //gdk_window_get_size(window->window, &a, &s);
            
    //gtk_window_get_size(GTK_WINDOW(win->toplevel), &w, &h);
    //printf("22222222222222222222222 %d, %d\n\n", w, h);
    //printf("33333333333333333333333 %d, %d\n\n", a, s);
    
    g_free(host);
    return 0;
}
static void login_engine_2(void *data)
{
    cJSON *recvInfo = data;
    int status;
    
    if(NULL == recvInfo)
    {
        dialog_for_message("返回信息异常，解析失败!");
        return ;
    }

    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        login_success();
        
    } else {
        
        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
        update_login_lab_text(_("欢迎使用凝思虚拟化产品!"));
        update_login_bun_status(NULL);
    }
    cJSON_Delete(recvInfo);
}
static void quit_login(void)
{
    char *host;
    
    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/usr/logout");

    g_thread_new("get_logout", curl_send_get, "get_logout");

    g_free(host);
}
static int logout(void *data)    
{
    GtkWidget *login;
    conn_info *ci;
    GtkTreeStore *store;
    GtkTreeModel *model;
    GtkTreeIter  iter;
    GtkTreeSelection *select;

    printf("\nwwwwwwwww  logout()   wwwwwwwwwwww\n\n");

    if(!IS_LOGIN)
        return 0;
    IS_LOGIN = FALSE;
    if(connections > 0)
    {
        if(NULL != win->conn)
        {
            win->conn->wins[win->id * CHANNELID_MAX + win->monitor_id] = NULL;
            win->conn = NULL;
        }
        while(conn_list != NULL)
        {
            ci = conn_list->data;
            g_signal_handlers_block_by_func(ci->channel, G_CALLBACK(main_channel_event), ci->conn);
            g_signal_handlers_block_by_func(ci->channel, G_CALLBACK(main_agent_update), ci->conn);
            connection_disconnect(ci->conn);
            conn_list = g_slist_remove(conn_list, ci);
            g_free(ci->vmName);
            g_free(ci);
        }
    } 
    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(win->vmlist));
    g_signal_handlers_block_by_func(win->vmbar, G_CALLBACK(vmbar_switch_page), win);
    g_signal_handlers_block_by_func(G_OBJECT(select), G_CALLBACK(vm_list_selection_changed), NULL);

    login = gtk_notebook_get_nth_page(GTK_NOTEBOOK(win->vmbar), 0);
    gtk_widget_show(login);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(win->vmbar), 0);
    int len=gtk_notebook_get_n_pages(GTK_NOTEBOOK(win->vmbar));
    for(int i=len-1; i>0; i--)
    {    
        gtk_notebook_remove_page(GTK_NOTEBOOK(win->vmbar), i);
    }

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(win->vmlist));
    store = GTK_TREE_STORE(model);
    if (gtk_tree_model_get_iter_first(model, &iter) == FALSE) 
    {
        global_free();
        return -1;
    }
    gtk_tree_store_clear(store);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, GTK_STOCK_HOME, 1, "Origin", -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, GTK_STOCK_PRINT_PREVIEW, 1, "Computers", -1);

    gtk_widget_queue_draw(win->vmbar);
    gtk_widget_queue_draw(win->vmlist);
    g_signal_handlers_unblock_by_func(win->vmbar, G_CALLBACK(vmbar_switch_page), win);
    g_signal_handlers_unblock_by_func(G_OBJECT(select), G_CALLBACK(vm_list_selection_changed), NULL);

    update_toolbar_status(-1);
    global_free();

    return 0;
}
static void quit_login_1(void *data)
{   
    cJSON *recvInfo = data;
    int status;
    
    if(NULL == recvInfo)
    {
        dialog_for_message("返回信息异常，解析失败!");
        return ;
    }

    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        logout(NULL);
        
    } else {
        
        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }

    cJSON_Delete(recvInfo);
}

static int get_allVmInfo(void *data)
{
    char *host;
    
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return -1;
    }   
    
    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/allvminfo?");  
    g_string_append(url, "_search=false&nd=1514358456936&rows=100&page=1&sidx=vmName&sord=desc"); 

    pthread_mutex_unlock(&lock);
    g_thread_new("get_all", curl_send_get, "get_all");
    
    g_free(host);
    return 0;
}
static int update_vmlist(gpointer data)
{
    GSList *list = data;
    GSList *list1 = list;
    GtkTreeModel *model;
    GtkTreeStore *store;
    GtkTreeSelection *select;
    GtkTreeIter iter, parent;
    char *vmName, *vm=NULL;
    int status, index;
    int flag=0;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(win->vmlist));
    store = GTK_TREE_STORE(model);
    
    if(FALSE == gtk_tree_model_get_iter_first(model, &parent))
        return -1;
    do{
        if(gtk_tree_model_iter_has_child(model, &parent))
        { 
            gtk_tree_model_iter_nth_child(model, &iter, &parent, 0);
            do{
                list = list1;
                flag = 0;
                gtk_tree_model_get(model, &iter, 1, &vmName, -1);
                while(list != NULL)
                {
                    vm = (char*)list->data;
                    if(0 == g_strcmp0(vm, vmName))
                    {
                        flag = 1;
                        break ;
                    }
                    list = g_slist_next(list);
                }
                if(0 == flag)
                {
                    index = get_vmbar_index(vmName);
                    if(index > 0)
                        gtk_notebook_remove_page(GTK_NOTEBOOK(win->vmbar), index);
                    
                    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(win->vmlist));
                    g_signal_handlers_block_by_func(G_OBJECT(select), G_CALLBACK(vm_list_selection_changed), NULL);
                    gtk_tree_store_remove(store, &iter);
                    g_signal_handlers_unblock_by_func(G_OBJECT(select), G_CALLBACK(vm_list_selection_changed), NULL);
                }
                g_free(vmName);
            }while(gtk_tree_model_iter_next(model, &iter));
        }    
    }while(gtk_tree_model_iter_next(model, &parent));

    gtk_tree_model_get_iter_from_string(model, &parent, "1");                                    
    list = list1;
    while(list != NULL)
    {
        vmName = (char*)list->data;      
        status = get_vm_status(vmName);
  
        if( get_vmlist_index(vmName) > 0)
        {
            update_vmlist_status(vmName, status);
            list = g_slist_next(list);
            continue ;
        } 
    
        gtk_tree_store_append(store, &iter, &parent);
        switch(status)
        {
            case 1:
                gtk_tree_store_set(store, &iter, 0, GTK_STOCK_MEDIA_PLAY, 1, vmName, -1);
                break;
            case 2:
                gtk_tree_store_set(store, &iter, 0, GTK_STOCK_MEDIA_PLAY, 1, vmName, -1);
                break;
            case 3:
                gtk_tree_store_set(store, &iter, 0, GTK_STOCK_MEDIA_PAUSE, 1, vmName, -1);
                break;
            default:
                gtk_tree_store_set(store, &iter, 0, GTK_STOCK_MEDIA_STOP, 1, vmName, -1);
                break;     
        }
        list = g_slist_next(list);
    }

    g_slist_free(list1);   
    return 0;
}
static void get_allVmInfo_1(void *data)
{
    cJSON *recvInfo = data;
    cJSON *vms, *tmp;
    char *vmName;
    int status;
    
    if(NULL == recvInfo)
    {    
        dialog_for_message("返回信息异常，解析失败!");
        pthread_mutex_unlock(&lock);
        return ;
    }
  
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        vms = cJSON_GetObjectItem(recvInfo, "rows");
        if(NULL == vms)
        {
            cJSON_Delete(recvInfo);
            return ;
        }
        if(NULL != allVmInfo)
            cJSON_Delete(allVmInfo);
        allVmInfo = recvInfo;

        GSList *list = NULL;
        for(int i=0; i<cJSON_GetArraySize(vms); i++)
        {   
            tmp = cJSON_GetArrayItem(vms, i);
            vmName = get_cjson_data(tmp, "vmName");
            list = g_slist_append(list, vmName);
        }  
        
        update_vmlist(list);
    } else {
        
        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
        cJSON_Delete(recvInfo);
    }
    return ;
}

static void connect_vm(gpointer data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return ;
    }
    
    char *host, *vmName=data, *vmId;
    
    g_source_remove(timer);
    
    strcpy(current_conn_vm, vmName);
    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    vmId = get_cjson_data(vmid_list, vmName);

    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/connectVm");
    params = g_string_new("vmId=");
    g_string_append(params, vmId);

    pthread_mutex_unlock(&lock);
    g_thread_new("post_conn_vm", curl_send_post, "post_conn_vm");

    g_free(host);
}
static void connect_vm_1(gpointer data)
{
    int status;
    cJSON *recvInfo = data;
    cJSON *connVm;

    if(NULL == recvInfo)
    {
        dialog_for_message("返回信息异常，解析失败!");
        return ;
    }
    
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        connVm = cJSON_GetObjectItem(recvInfo, "data");
        if(NULL == connVm)
        {  
            cJSON_Delete(recvInfo);
            return ;
        }
        connect_vm_go(connVm);
        
    } else {

        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }
    
    cJSON_Delete(recvInfo);
}

static int connect_vm_go(void *data)
{
    cJSON *connectVm = data;
    char *hostIp=NULL, *tls_port=NULL, *passwd=NULL;

    //g_print("conn go : %s\n", cJSON_PrintUnformatted(connectVm));
    
    hostIp = get_cjson_data(connectVm, "ip");
    tls_port = get_cjson_data(connectVm, "tls-port");
    passwd = get_cjson_data(connectVm, "passwd");

    spice_connection *conn;
    conn = connection_new();
    spice_set_session_option(conn->session);

    g_object_set(conn->session, "host", hostIp, NULL);
    g_object_set(conn->session, "tls-port", tls_port, NULL);
    g_object_set(conn->session, "password", passwd, NULL);

    connection_connect(conn);
    return 0;
}

static void while_conn_shutdown(spice_connection *data)
{
    spice_connection *conn = data;
    GtkWidget *window;
    conn_info *ci;
    char *vmName;
    int index, status;
                    
    ci = get_conn_from_list_by_conn(conn); 
    g_signal_handlers_block_by_func(win->vmbar, G_CALLBACK(vmbar_switch_page), win);
    
    if(NULL != ci)
    {
        vmName = ci->vmName;
        index = get_vmbar_index(vmName);
        ci->conn->wins[ci->id * CHANNELID_MAX + ci->monitor_id] = NULL;
        g_signal_handlers_block_by_func(ci->channel, G_CALLBACK(main_channel_event), ci->conn);
        g_signal_handlers_block_by_func(ci->channel, G_CALLBACK(main_agent_update), ci->conn);
        connection_disconnect(ci->conn);
        conn_list = g_slist_remove(conn_list, ci);

    } else {

        index = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->vmbar));
        vmName = get_vmbar_vmname(index); 
    }
 
    if(IS_VM_PAUSE) 
    {
        status = 3;
        window = vm_set_diswindow(vmName, status);
    }
    else
    {
        status = 0;
        window = vm_set_diswindow(vmName, status);
    }

    if(NULL == window)
    {
        update_vmlist_status(vmName, status);
        update_toolbar_status(status);
        IS_VM_PAUSE = FALSE;
        dialog_for_message(_("获取虚拟机信息失败!"));
        if(NULL != ci)
        {
            g_free(ci->vmName);
            g_free(ci);
        } 
        return ;
    }

    gtk_widget_set_size_request(win->toplevel, 1100, 820);
    gtk_widget_show_all(window);
    gtk_notebook_insert_page(GTK_NOTEBOOK(win->vmbar), window, linx_noetbook_tab_label(vmName), index);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(win->vmbar), index);
    gtk_notebook_remove_page(GTK_NOTEBOOK(win->vmbar), index+1);
    gtk_widget_queue_draw(win->vmbar);
    g_signal_handlers_unblock_by_func(win->vmbar, G_CALLBACK(vmbar_switch_page), win);
    gtk_widget_queue_resize_no_redraw(win->toplevel);
  
    update_vmlist_status(vmName, status);
    update_toolbar_status(status);
    IS_VM_PAUSE = FALSE;
  
    if(NULL != ci)
    {
       g_free(ci->vmName);
       g_free(ci);
    } 
}

static GtkWidget* get_vm_info_tree(char *vm)
{
    GtkWidget *tree;
    GtkTreeViewColumn *column, *column1, *column2;
    GtkCellRenderer *renderer_txt, *renderer_pixbuf;
    GtkTreeStore *store;
    GtkTreeIter iter;
    
    cJSON *recvInfo, *vmInfo;
    char *id, *mac, *protocol, *video, *memSize, *cpu, *c_cpu,
           *t_cpu, *imgId, *temp;
    int status;
    GThread *td;
    
    td = g_thread_new("vm_id", get_vm_info, vm);
    g_thread_join(td);
    recvInfo = cjson_recv;
    if(NULL == recvInfo)
        return NULL;

    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 != status)
    {
        cJSON_Delete(recvInfo); 
        if(-6 == status)
            dialog_for_message(_("登录异常\n连接已断开!"));
        return NULL;
    }

    vmInfo = cJSON_GetObjectItem(recvInfo, "vmInfo");
    id = get_cjson_data(vmInfo, "vmId");
    mac = get_cjson_data(vmInfo, "macAddr");
    protocol = get_cjson_data(vmInfo, "graphicProtocol");
    video = get_cjson_data(vmInfo, "graphicType");
    memSize = get_cjson_data(vmInfo, "memSize");
    cpu = get_cjson_data(vmInfo, "cpuNum");
    c_cpu = get_cjson_data(vmInfo, "coresPerCpu");
    t_cpu = get_cjson_data(vmInfo, "threadsPerCore");
    imgId = get_cjson_data(vmInfo, "imageUUID");
    temp = get_cjson_data(vmInfo, "tempName");

    store = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, GTK_STOCK_INFO, 1, "虚拟机ID", 2, id, -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, GTK_STOCK_INFO, 1, "MAC地址", 2, mac, -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, GTK_STOCK_INFO, 1, "虚拟机磁盘ID", 2, imgId, -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, GTK_STOCK_INFO, 1, "基于的模板", 2, temp, -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, GTK_STOCK_INFO, 1, "图形界面协议", 2, protocol, -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, GTK_STOCK_INFO, 1, "视频协议", 2, video, -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, GTK_STOCK_INFO, 1, "内存大小(MB)", 2, memSize, -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, GTK_STOCK_INFO, 1, "虚拟CPU总数", 2, cpu, -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, GTK_STOCK_INFO, 1, "单个CPU内核数", 2, c_cpu, -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, GTK_STOCK_INFO, 1, "每个内核线程数", 2, t_cpu, -1);
    
    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

    renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
    renderer_txt = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer_txt), "foreground", "black", NULL);
    column = gtk_tree_view_column_new_with_attributes("",
                                                       renderer_pixbuf,
                                                       "stock-id",
                                                       0,
                                                       NULL);
    column1 = gtk_tree_view_column_new_with_attributes("",
                                                       renderer_txt,
                                                       "text",
                                                       1,
                                                       NULL);
    column2 = gtk_tree_view_column_new_with_attributes("",
                                                       renderer_txt,
                                                       "text",
                                                       2,
                                                       NULL);
    
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column2);

    g_object_unref(store);
    cJSON_Delete(recvInfo);
    return tree;
}
static GtkWidget* vm_set_diswindow(char *vm, int vm_status)
{
    GtkWidget *win, *name, *sep, *bun, *vbox, *vbox1, *vbox2, 
                                   *hb, *hbox, *hbox1, *hbox2, *hbox3;
    GtkWidget *notes, *description, *info, *info_tree, *image, *s_image;
    GString *img_file;
    GtkTextBuffer *buffer;
    GdkPixbuf *src, *des;
    int status, width, height;
    char *c_notes, span[128];

    status = vm_status;
    name = gtk_label_new(NULL);
    info = gtk_label_new("虚拟机基础信息:");
    sep = gtk_hseparator_new();
    bun = new_image_bun(GTK_STOCK_MEDIA_PLAY, _("启动该虚拟机"));
    notes = gtk_label_new(_("虚拟机注释:"));
    description = gtk_text_view_new();
    c_notes = get_vm_property(vm, "description");
    info_tree = get_vm_info_tree(vm);
    if(NULL == info_tree)
        return NULL;
    sprintf(span, "<span foreground='blue' font_desc='22'>%s</span>", vm);
    gtk_label_set_markup(GTK_LABEL(name), span);
    gtk_widget_set_sensitive(description, FALSE);
    gtk_widget_set_size_request(description, 100, 150);
    
    win = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 0);
    vbox1 = gtk_vbox_new(FALSE, 0);
    vbox2 = gtk_vbox_new(FALSE, 0);
    hb = gtk_hbox_new(FALSE, 0);
    hbox = gtk_hbox_new(FALSE, 0);
    hbox1 = gtk_hbox_new(FALSE, 0);
    hbox2 = gtk_hbox_new(FALSE, 0);
    hbox3 = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox1), 5);
    gtk_container_set_border_width(GTK_CONTAINER(win), 3);
    
    gtk_container_add(GTK_CONTAINER(win), vbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(hbox1), vbox1, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox1), bun, FALSE, FALSE, 50);
    gtk_box_pack_start(GTK_BOX(hb), info, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox1), hb, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox1), info_tree, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(hbox3), notes, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox2), hbox3, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(vbox2), description, FALSE, FALSE, 3);
    gtk_box_pack_start(GTK_BOX(hbox2), vbox2, TRUE, TRUE, 0);
    
    img_file = g_string_new("/etc/linx/ppms/");
    if(IS_VM_PAUSE)
    {
        s_image = gtk_image_new_from_file("/etc/linx/ppms/png/pause0.png");
        g_string_append(img_file, "ppm/");
        g_string_append(img_file, vm);
        g_string_append(img_file, ".ppm"); 
    } else {

        if(3 == status)
        {
            s_image = gtk_image_new_from_file("/etc/linx/ppms/png/pause0.png");
            g_string_append(img_file, "png/pause1.png"); 
            
        } else {
            
            s_image = gtk_image_new_from_file("/etc/linx/ppms/png/stop0.png");
            g_string_append(img_file, "png/stop1.png"); 
        }
    }
    src = gdk_pixbuf_new_from_file(img_file->str, NULL);
    width = gdk_pixbuf_get_width(src);
    height = gdk_pixbuf_get_height(src);
    
    if(IS_VM_PAUSE)
    {
        if(width > 1000)
            des = gdk_pixbuf_scale_simple(src, (width/5)*3, (height/5)*3, GDK_INTERP_BILINEAR);
        else
            des = gdk_pixbuf_scale_simple(src, (width/4)*3, (height/4)*3, GDK_INTERP_BILINEAR);
        IS_VM_PAUSE = FALSE;
    }
    else
        des = gdk_pixbuf_scale_simple(src, 500, 400, GDK_INTERP_BILINEAR);
    image = gtk_image_new_from_pixbuf(des); 

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(description));
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), c_notes, -1);
   
    gtk_box_pack_start(GTK_BOX(hbox), s_image, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), name, FALSE, FALSE, 5); 
    gtk_box_pack_start(GTK_BOX(hbox1), image, TRUE, TRUE, 10); 

    g_signal_connect(bun, "clicked", G_CALLBACK(menu_cb_StartVm), NULL);
    
    g_object_unref(src);
    g_object_unref(des);
    g_string_free(img_file, TRUE);
    return win;
}

static int ask_user(GtkWidget *parent, char *title, char *message,
                    char *dest, int dlen, int hide)
{
    GtkWidget *dialog, *area, *label, *entry;
    const char *txt;
    int retval;

    /* Create the widgets */
    dialog = gtk_dialog_new_with_buttons(title,
                                         parent ? GTK_WINDOW(parent) : NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_OK,
                                         GTK_RESPONSE_ACCEPT,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_REJECT,
                                         NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    label = gtk_label_new(message);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(area), label, FALSE, FALSE, 5);

    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), dest);
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    if (hide)
        gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_box_pack_start(GTK_BOX(area), entry, FALSE, FALSE, 5);

    /* show and wait for response */
    gtk_widget_show_all(dialog);
    switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
    case GTK_RESPONSE_ACCEPT:
        txt = gtk_entry_get_text(GTK_ENTRY(entry));
        snprintf(dest, dlen, "%s", txt);
        retval = 0;
        break;
    default:
        retval = -1;
        break;
    }
    gtk_widget_destroy(dialog);
    return retval;
}

static struct {
    const char *text;
    const char *prop;
    GtkWidget *entry;
} connect_entries[] = {
    { .text = N_("Hostname"),   .prop = "host"      },
    { .text = N_("Port"),       .prop = "port"      },
    { .text = N_("TLS Port"),   .prop = "tls-port"  },
};

#ifndef G_OS_WIN32
static void recent_selection_changed_dialog_cb(GtkRecentChooser *chooser, gpointer data)
{
    GtkRecentInfo *info;
    gchar *txt = NULL;
    const gchar *uri;
    SpiceSession *session = data;

    info = gtk_recent_chooser_get_current_item(chooser);
    if (info == NULL)
        return;

    uri = gtk_recent_info_get_uri(info);
    g_return_if_fail(uri != NULL);

    g_object_set(session, "uri", uri, NULL);

    g_object_get(session, "host", &txt, NULL);
    gtk_entry_set_text(GTK_ENTRY(connect_entries[0].entry), txt ? txt : "");
    g_free(txt);

    g_object_get(session, "port", &txt, NULL);
    gtk_entry_set_text(GTK_ENTRY(connect_entries[1].entry), txt ? txt : "");
    g_free(txt);

    g_object_get(session, "tls-port", &txt, NULL);
    gtk_entry_set_text(GTK_ENTRY(connect_entries[2].entry), txt ? txt : "");
    g_free(txt);

    gtk_recent_info_unref(info);
}

static void recent_item_activated_dialog_cb(GtkRecentChooser *chooser, gpointer data)
{
   gtk_dialog_response (GTK_DIALOG (data), GTK_RESPONSE_ACCEPT);
}
#endif

static int connect_dialog(SpiceSession *session)
{
    GtkWidget *dialog, *area, *label;
    GtkTable *table;
    int i, retval;

    /* Create the widgets */
    dialog = gtk_dialog_new_with_buttons(_("Connect to Linx Virtual machine"),
                                         NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_REJECT,
                                         GTK_STOCK_CONNECT,
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    table = GTK_TABLE(gtk_table_new(3, 2, 0));
    gtk_box_pack_start(GTK_BOX(area), GTK_WIDGET(table), TRUE, TRUE, 0);
    gtk_table_set_row_spacings(table, 5);
    gtk_table_set_col_spacings(table, 5);


    for (i = 0; i < SPICE_N_ELEMENTS(connect_entries); i++) {
        gchar *txt;
        label = gtk_label_new(connect_entries[i].text);
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_table_attach_defaults(table, label, 0, 1, i, i+1);
        connect_entries[i].entry = GTK_WIDGET(gtk_entry_new());
        gtk_table_attach_defaults(table, connect_entries[i].entry, 1, 2, i, i+1);
        g_object_get(session, connect_entries[i].prop, &txt, NULL);
        SPICE_DEBUG("%s: #%i [%s]: \"%s\"",
                __FUNCTION__, i, connect_entries[i].prop, txt);
        if (txt) {
            gtk_entry_set_text(GTK_ENTRY(connect_entries[i].entry), txt);
            g_free(txt);
        }
    }

    label = gtk_label_new("Recent connections:");
    gtk_box_pack_start(GTK_BOX(area), label, TRUE, TRUE, 0);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
#ifndef G_OS_WIN32
    GtkRecentFilter *rfilter;
    GtkWidget *recent;

    recent = GTK_WIDGET(gtk_recent_chooser_widget_new());
    gtk_recent_chooser_set_show_icons(GTK_RECENT_CHOOSER(recent), FALSE);
    gtk_box_pack_start(GTK_BOX(area), recent, TRUE, TRUE, 0);

    rfilter = gtk_recent_filter_new();
    gtk_recent_filter_add_mime_type(rfilter, "application/x-spice");
    gtk_recent_chooser_set_filter(GTK_RECENT_CHOOSER(recent), rfilter);
    gtk_recent_chooser_set_local_only(GTK_RECENT_CHOOSER(recent), FALSE);
    g_signal_connect(recent, "selection-changed",
                     G_CALLBACK(recent_selection_changed_dialog_cb), session);
    g_signal_connect(recent, "item-activated",
                     G_CALLBACK(recent_item_activated_dialog_cb), dialog);
#endif

    /* show and wait for response */
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        for (i = 0; i < SPICE_N_ELEMENTS(connect_entries); i++) {
            const gchar *txt;
            txt = gtk_entry_get_text(GTK_ENTRY(connect_entries[i].entry));
            g_object_set(session, connect_entries[i].prop, txt, NULL);
        }
        retval = 0;
    } else
        retval = -1;
    gtk_widget_destroy(dialog);
    return retval;
}

/* ------------------------------------------------------------------ */

static void update_status_window(SpiceWindow *win)
{
    gchar *status;

    if (win == NULL)
        return;
    
    if (win->mouse_grabbed) {
        SpiceGrabSequence *sequence = spice_display_get_grab_keys(SPICE_DISPLAY(win->spice));
        gchar *seq = spice_grab_sequence_as_string(sequence);
        status = g_strdup_printf(_("Use %s to ungrab mouse."), seq);
        g_free(seq);
    } else {   
        status = g_strdup_printf(_("mouse: %s, agent: %s"),
                 win->conn->mouse_state, win->conn->agent_state);
    }
    gtk_label_set_text(GTK_LABEL(win->status), status);
    
 g_print("update_status_windo:123 end \n");
    g_free(status);
}

static void update_status(struct spice_connection *conn)
{
    int i;

    for (i = 0; i < SPICE_N_ELEMENTS(conn->wins); i++) {
        if (conn->wins[i] == NULL)
            continue;
        update_status_window(conn->wins[i]);
    }

    g_print("update_status(): \n");
}

static const char *spice_edit_properties[] = {
    "CopyToGuest",
    "PasteFromGuest",
};

static void update_edit_menu_window(SpiceWindow *win)
{
    int i;
    GtkAction *toggle;

    if (win == NULL) {
        return;
    }

    /* Make "CopyToGuest" and "PasteFromGuest" insensitive if spice
     * agent is not connected */
    for (i = 0; i < G_N_ELEMENTS(spice_edit_properties); i++) {
        toggle = gtk_action_group_get_action(win->ag, spice_edit_properties[i]);
        if (toggle) {
            gtk_action_set_sensitive(toggle, win->conn->agent_connected);
        }
    }
}

static void update_edit_menu(struct spice_connection *conn)
{
    int i;

    for (i = 0; i < SPICE_N_ELEMENTS(conn->wins); i++) {
        if (conn->wins[i]) {
            update_edit_menu_window(conn->wins[i]);
        }
    }
}

static void menu_cb_connect(GtkAction *action, void *data)
{
    struct spice_connection *conn;

    conn = connection_new();
    connection_connect(conn);
}

static void menu_cb_close(GtkAction *action, void *data)
{
    SpiceWindow *win = data;

    connection_disconnect(win->conn);
}

static void menu_cb_copy(GtkToolButton *toolbutton, void *data)
{
    SpiceWindow *win = data;

    spice_gtk_session_copy_to_guest(win->conn->gtk_session);
}

static void menu_cb_paste(GtkToolButton *toolbutton, void *data)
{
    SpiceWindow *win = data;

    spice_gtk_session_paste_from_guest(win->conn->gtk_session);
}

static void window_set_fullscreen(SpiceWindow *win, gboolean fs)
{
    if (fs) {
#ifdef G_OS_WIN32
        gtk_window_get_position(GTK_SINDOW(win->toplevel), &win->win_x, &win->win_y);
#endif
        gtk_window_fullscreen(GTK_WINDOW(win->toplevel));
    } else {
        gtk_window_unfullscreen(GTK_WINDOW(win->toplevel));
#ifdef G_OS_WIN32
        gtk_window_move(GTK_WINDOW(win->toplevel), win->win_x, win->win_y);
#endif
    }
}

static void dialog_for_update_passwd(char *userNmae, char *pswd)
{
    GtkWidget *dialog, *area, *seq;
    GtkWidget *user, *user1, *passwd, *passwd1, *n_passwd, *n_passwd1;
    GtkWidget *u_hbox, *p_hbox, *p_hbox1;
    char *pd, *pd1;
        
    user = gtk_label_new(_("当前用户"));
    passwd = gtk_label_new(_("新  密   码"));
    n_passwd = gtk_label_new(_("确认密码"));
    user1 = gtk_entry_new();
    passwd1 = gtk_entry_new();
    n_passwd1 = gtk_entry_new();
    u_hbox = gtk_hbox_new(FALSE, 2);
    p_hbox = gtk_hbox_new(FALSE, 2);
    p_hbox1 = gtk_hbox_new(FALSE, 2);

    gtk_entry_set_text(GTK_ENTRY(user1), userNmae);
    gtk_widget_set_sensitive(user1, FALSE);
    gtk_entry_set_visibility(GTK_ENTRY(passwd1), FALSE);
    gtk_entry_set_visibility(GTK_ENTRY(n_passwd1), FALSE);
    gtk_entry_set_max_length(GTK_ENTRY(passwd1), 32);
    gtk_entry_set_max_length(GTK_ENTRY(n_passwd1), 32);
    
    gtk_box_pack_start(GTK_BOX(u_hbox), user, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(u_hbox), user1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(p_hbox), passwd, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(p_hbox), passwd1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(p_hbox1), n_passwd, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(p_hbox1), n_passwd1, TRUE, TRUE, 5);

    dialog = gtk_dialog_new_with_buttons(_("修改用户密码"), GTK_WINDOW(win->toplevel),
                                    GTK_DIALOG_MODAL,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_OK, GTK_RESPONSE_OK,
                                    NULL);
    seq = gtk_hseparator_new();
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
    gtk_widget_set_size_request(dialog, 350, 250);   
    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(area), u_hbox, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), p_hbox, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), p_hbox1, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), seq, TRUE, TRUE, 10);
    
    gtk_widget_show_all(dialog);

    int flag;
    while( TRUE )
    {
        flag = 1;
        switch(gtk_dialog_run(GTK_DIALOG(dialog)))
        {
            case GTK_RESPONSE_OK:
                pd = (char*)gtk_entry_get_text(GTK_ENTRY(passwd1));
                pd1 = (char*)gtk_entry_get_text(GTK_ENTRY(n_passwd1));
                
                if(0 == g_strcmp0(pd, "") || 0 == g_strcmp0(pd1, ""))
                {
                    dialog_for_message(_("请输入密码!"));
                    break ;
                }
                if(0 != g_strcmp0(pd, pd1))
                {
                    dialog_for_message(_("两次输入密码不一致!"));
                    break ;
                }
                strcpy(pswd, pd);
                flag = 0;
                break ;       
            default:
                strcpy(pswd, "");
                flag = 0;
                break ;
        }   
        if(0 == flag)
            break ;
    }

    gtk_widget_destroy(dialog);
    return ;   
}
static void menu_cb_UpdatePasswd(GtkToolButton *toolbutton, void *data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return ;
    }
    g_source_remove(timer);
    
    char *host, *user, passwd[32];
        
    user = (char*)gtk_entry_get_text(GTK_ENTRY(wgt.userName_entry));
    dialog_for_update_passwd(user, passwd);
    if(0 == g_strcmp0(passwd, ""))
    {
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return;
    }
    
    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry)); 
    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/usr/modifypassword");

    params = g_string_new("username=");
    g_string_append(params, user);
    g_string_append(params, "&password=");
    g_string_append(params, passwd);

    pthread_mutex_unlock(&lock);
    g_thread_new("post_update_passwd", curl_send_post, "post_update_passwd");

    g_free(host);
    dialog_for_progress(_("正在修改用户密码:"));
}
static void updatePasswd(void *data)
{
    cJSON *recvInfo = data;
    int status;
    
    close_progress_dialog(NULL);
    if(NULL == recvInfo)
    {
        dialog_for_message("返回信息异常，解析失败!");
        return ;
    }

    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        dialog_for_message(_("密码修改成功!"));
        
    } else {

        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }

    cJSON_Delete(recvInfo);
}

static void menu_cb_fullscreen(GtkToolButton *toolbutton, void *data)
{
    SpiceWindow *win = data;

    window_set_fullscreen(win, !win->fullscreen);
}

static cJSON* dialog_for_newImage(cJSON *data)
{
    GtkWidget *dialog, *area, *seq;
    cJSON *result = NULL, *domains = data, *tmp;
    GString *domain;
    char *domain_name, *domain_free, *domain_total;
    
    GtkWidget *name, *notes, *size, *storage, *policy;
    GtkWidget *name1, *notes1, *size1, *storage1, *policy1;
    GtkWidget *hbox_name, *hbox_notes, *hbox_size, *hbox_storage, *hbox_policy;
    const char *c_name, *c_notes, *c_size;
    char *c_storage, *c_policy;
    
    name = gtk_label_new(_("名                 称"));
    notes = gtk_label_new(_("注                 释"));
    size = gtk_label_new(_("大        小(GB)"));
    storage = gtk_label_new(_("存储域(空闲/总空间)"));
    policy = gtk_label_new(_("分 配 策 略"));

    name1 = gtk_entry_new();
    notes1 = gtk_entry_new();
    size1 = gtk_entry_new();
    storage1 = gtk_combo_box_text_new();
    policy1 = gtk_combo_box_text_new();

    gtk_entry_set_max_length(GTK_ENTRY(name1), 32);
    gtk_entry_set_max_length(GTK_ENTRY(notes1), 128);
    gtk_entry_set_max_length(GTK_ENTRY(size1), 5);
    g_signal_connect(G_OBJECT(size1), "insert-text", G_CALLBACK(on_entry_insert_text), NULL);
    
    hbox_name = gtk_hbox_new(FALSE, 2);
    hbox_notes = gtk_hbox_new(FALSE, 2);
    hbox_size = gtk_hbox_new(FALSE, 2);
    hbox_storage = gtk_hbox_new(FALSE, 2);
    hbox_policy = gtk_hbox_new(FALSE, 2);

    gtk_box_pack_start(GTK_BOX(hbox_name), name, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_name), name1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_notes), notes, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_notes), notes1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_size), size, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_size), size1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_storage), storage, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_storage), storage1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_policy), policy, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_policy), policy1, TRUE, TRUE, 5);

    if(NULL != domains)
    {
        for(int i=0; i<cJSON_GetArraySize(domains); i++)
        {
            tmp = cJSON_GetArrayItem(domains, i);
            domain_name = get_cjson_data(tmp, "storageName");
            domain_free = get_cjson_data(tmp, "freeSize");
            domain_total = get_cjson_data(tmp, "totalSize");

            domain = g_string_new(domain_name);
            g_string_append(domain, "(");
            g_string_append(domain, domain_free);
            g_string_append(domain, "/");
            g_string_append(domain, domain_total);
            g_string_append(domain, ")");
        
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(storage1), domain->str);
            g_string_free(domain, TRUE);
        }
    }

    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(policy1), _("Thin Provision"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(policy1), _("Preallocated"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(storage1), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(policy1), 0);
    
    seq = gtk_hseparator_new();
    dialog = gtk_dialog_new_with_buttons(_("新建磁盘"), GTK_WINDOW(win->toplevel),
                                    GTK_DIALOG_MODAL,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_OK, GTK_RESPONSE_OK,
                                    NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
    gtk_widget_set_size_request(dialog, 400, 350);   
    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(area), hbox_name, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_notes, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_size, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_storage, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_policy, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), seq, TRUE, TRUE, 10);
    
    gtk_widget_show_all(dialog);

    int flag;
    while( TRUE )
    {
        flag = 1;
        switch(gtk_dialog_run(GTK_DIALOG(dialog)))
        {
            case GTK_RESPONSE_OK:
                c_name = gtk_entry_get_text(GTK_ENTRY(name1));
                c_notes = gtk_entry_get_text(GTK_ENTRY(notes1));
                c_size = gtk_entry_get_text(GTK_ENTRY(size1));
                c_storage = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(storage1));
                c_policy = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(policy1));
                if(0 == g_strcmp0(c_name, "") || 0 == g_strcmp0(c_size, ""))
                {
                    dialog_for_message(_("磁盘名称或大小不能为空!"));
                    break ;
                }
                result = cJSON_CreateObject();
                cJSON_AddStringToObject(result, "name", c_name);
                cJSON_AddStringToObject(result, "notes", c_notes);
                cJSON_AddStringToObject(result, "size", c_size);
                for(int i=0; i<strlen(c_storage); i++)
                {
                    if('(' == c_storage[i])
                    {
                        c_storage[i] = '\0';
                        break;
                    }
                }
                cJSON_AddStringToObject(result, "storage", c_storage);
                if(strstr(c_policy, "Preallocated"))  
                    cJSON_AddStringToObject(result, "policy", "1");
                else 
                    cJSON_AddStringToObject(result, "policy", "2");
                
                flag = 0;
                g_free(c_storage);
                g_free(c_policy);
                break ;       
            default:
                flag = 0;
                break ;
        }   
        if(0 == flag)
            break ;
    }
    gtk_widget_destroy(dialog);
    return result;
}
static void image_list_select(GtkTreeSelection *treeselection, gpointer data)
{
    GtkWidget *image=(GtkWidget *)data;
    GtkTreeIter iter;
    GtkTreeModel* model;
    char *name, *id;

    if(gtk_tree_selection_get_selected(treeselection, &model, &iter))
    {
        gtk_tree_model_get(model, &iter, 0, &name, 2, &id, -1);
    }
    strcpy(new_vm_imgId, id);
    gtk_entry_set_text(GTK_ENTRY(image), name);
    g_free(name);
    g_free(id);
    return ;
}
static void dialog_for_attachImage(cJSON *data, GtkWidget *image)
{
    GtkWidget *dialog, *area, *scrlled_window;
    cJSON *disks=data, *tmp;
    char *name, *notes, *id, *size, *storage;
    
    GtkWidget *tree;
    GtkTreeViewColumn *column1, *column2, *column3, *column4, *column5;
    GtkCellRenderer *renderer_txt;
    GtkTreeStore *store;
    GtkTreeIter iter;
    GtkTreeSelection *select;

    store = gtk_tree_store_new(5, G_TYPE_STRING, G_TYPE_STRING, 
                                    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    for(int i=0; i<cJSON_GetArraySize(disks); i++)
    {
        tmp = cJSON_GetArrayItem(disks, i);
        
        name = get_cjson_data(tmp, "diskAlias");
        notes = get_cjson_data(tmp, "description");
        id = get_cjson_data(tmp, "imageUUID");
        size = get_cjson_data(tmp, "size");
        storage = get_cjson_data(tmp, "domainName");
            
        gtk_tree_store_append(store, &iter, NULL);
        if(0 == g_strcmp0(notes, "null"))
            gtk_tree_store_set(store, &iter, 0, name, 1, "", 2, id,
                                                 3, size, 4, storage, -1);
        else
            gtk_tree_store_set(store, &iter, 0, name, 1, notes, 2, id,
                                                 3, size, 4, storage, -1);
    }
    
    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    renderer_txt = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer_txt), "foreground", "black", NULL);
    column1 = gtk_tree_view_column_new_with_attributes("名称",
                                                       renderer_txt,
                                                       "text",
                                                       0,
                                                       NULL);
    column2 = gtk_tree_view_column_new_with_attributes("注释",
                                                       renderer_txt,
                                                       "text",
                                                       1,
                                                       NULL);
    column3 = gtk_tree_view_column_new_with_attributes("磁盘ID",
                                                       renderer_txt,
                                                       "text",
                                                       2,
                                                       NULL);
    column4 = gtk_tree_view_column_new_with_attributes("大小(GB)",
                                                       renderer_txt,
                                                       "text",
                                                       3,
                                                       NULL);
    column5 = gtk_tree_view_column_new_with_attributes("存储域",
                                                       renderer_txt,
                                                       "text",
                                                       4,
                                                       NULL);


    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
    g_signal_connect(G_OBJECT(select),
                     "changed",
                     G_CALLBACK(image_list_select),
                     image);
    gtk_tree_view_column_set_resizable(column1, TRUE);
    gtk_tree_view_column_set_resizable(column2, TRUE);
    gtk_tree_view_column_set_resizable(column3, TRUE);
    gtk_tree_view_column_set_resizable(column4, TRUE);
    gtk_tree_view_column_set_resizable(column5, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column2);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column3);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column4);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column5);

    dialog = gtk_dialog_new_with_buttons(_("附加磁盘"), GTK_WINDOW(win->toplevel),
                                    GTK_DIALOG_MODAL,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_OK, GTK_RESPONSE_OK,
                                    NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);

    scrlled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(scrlled_window), 10);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrlled_window), 
                                        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrlled_window), tree);
    
    gtk_widget_set_size_request(dialog, 550, 350);  
    gtk_widget_set_size_request(tree, 550, 250);
    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(area), scrlled_window, TRUE, TRUE, 5); 
    
    gtk_widget_show(area);
    gtk_widget_show_all(dialog);

    switch(gtk_dialog_run(GTK_DIALOG(dialog)))
    {
        case GTK_RESPONSE_OK:
            break;
            
        default:
            gtk_entry_set_text(GTK_ENTRY(image), "");
            break ;
    }

    gtk_widget_destroy(dialog);
    gtk_tree_store_clear(store);
    return ;
}

static GtkWidget *up_bun, *down_bun;
static char *current_lead=NULL;
static void up_current_lead(GtkButton *button, gpointer *data)
{
    GtkTreeStore *store;
    GtkWidget *tree=GTK_WIDGET(data);
    GtkTreeIter iter, prv_iter;
    GtkTreeModel *model;
    char *lead;
    int i=0;

    gtk_widget_set_sensitive(up_bun, TRUE);
    gtk_widget_set_sensitive(down_bun, TRUE);
    
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
    store = GTK_TREE_STORE(model);
    gtk_tree_model_get_iter_first(model, &iter);   
    do
    {
        gtk_tree_model_get(model, &iter, 0, &lead, -1);
        if(0 == g_strcmp0(lead, current_lead))
        {
            g_free(lead);
            break ;
        }
        prv_iter = iter;
        g_free(lead);
    }while(gtk_tree_model_iter_next(model, &iter));

    gtk_tree_store_move_before(store, &iter, &prv_iter);

    gtk_tree_model_get_iter_first(model, &iter);
    i=0;
    do
    {
        gtk_tree_model_get(model, &iter, 0, &lead, -1);
        if(0 == g_strcmp0(lead, current_lead))
        {
            g_free(lead);
            break ;
        }
        i++;
        g_free(lead);
    }while(gtk_tree_model_iter_next(model, &iter));
    
    if(0 == i)
        gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
}
static void down_current_lead(GtkButton *button, gpointer *data)
{
    GtkTreeStore *store;
    GtkWidget *tree=GTK_WIDGET(data);
    GtkTreeIter iter, prv_iter;
    GtkTreeModel *model;
    char *lead;
    int i=0;

    gtk_widget_set_sensitive(up_bun, TRUE);
    gtk_widget_set_sensitive(down_bun, TRUE);
    
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
    store = GTK_TREE_STORE(model);
    gtk_tree_model_get_iter_first(model, &iter);  
    do
    {
        gtk_tree_model_get(model, &iter, 0, &lead, -1);
        if(0 == g_strcmp0(lead, current_lead))
        {   
            g_free(lead);
            prv_iter = iter;
            gtk_tree_model_iter_next(model, &iter);
            break ;
        }
        g_free(lead);
    }while(gtk_tree_model_iter_next(model, &iter));

    gtk_tree_store_move_after(store, &prv_iter, &iter);

    gtk_tree_model_get_iter_first(model, &iter);
    i=0;
    do
    {
        gtk_tree_model_get(model, &iter, 0, &lead, -1);
        if(0 == g_strcmp0(lead, current_lead))
        {
            g_free(lead);
            break ;
        }
        i++;
        g_free(lead);
    }while(gtk_tree_model_iter_next(model, &iter));
    
    if(2 == i)
        gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
}
static void runOnce_list_select(GtkTreeSelection *treeselection, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    char *lead;
    int i=0;

    if(current_lead)
    {
        g_free(current_lead);
        current_lead = NULL;
    }
    gtk_widget_set_sensitive(up_bun, TRUE);
    gtk_widget_set_sensitive(down_bun, TRUE);
    
    if(gtk_tree_selection_get_selected(treeselection, &model, &iter))
    {
        gtk_tree_model_get(model, &iter, 0, &lead, -1);
        current_lead = lead;
    }

    gtk_tree_model_get_iter_first(model, &iter);
    do
    {
        gtk_tree_model_get(model, &iter, 0, &lead, -1);     
        if(0 == g_strcmp0(lead, current_lead))
        {
            g_free(lead);
            break ;
        }
        i++;
        g_free(lead);
    }while(gtk_tree_model_iter_next(model, &iter));

    if(0 == i)
        gtk_widget_set_sensitive(up_bun, FALSE);
    if(2 == i)
        gtk_widget_set_sensitive(down_bun, FALSE);

    return ;
}
static cJSON * dialog_for_runOnce(cJSON *data)
{
    GtkWidget *dialog, *area, *seq;
    cJSON *isos=data, *result=NULL;
    char *iso=NULL;

    GtkWidget *cd, *cd1, *lead, *lead1;
    GtkWidget *hbox_cd, *hbox_lead, *vbox_bun;

    GtkTreeStore *store;
    GtkTreeModel *model;
    GtkTreeViewColumn* column;
    GtkCellRenderer* vm_renderer;
    GtkTreeIter iter;
    GtkTreeSelection* select;

    store = gtk_tree_store_new(1, G_TYPE_STRING);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, "硬盘", -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, "CD-ROM", -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, "网络(PXE)", -1);

    lead1 = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(lead1), FALSE);
    vm_renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(vm_renderer), "foreground", "black", NULL);
    column = gtk_tree_view_column_new_with_attributes("",
                                                       vm_renderer,
                                                       "text",
                                                       0,
                                                       NULL);
    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(lead1));
    gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
    g_signal_connect(G_OBJECT(select),
                     "changed",
                     G_CALLBACK(runOnce_list_select),
                     lead1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(lead1), column);
    
    cd = gtk_label_new(_("附 加 CD:"));
    lead = gtk_label_new(_("引导顺序:"));
    cd1 = gtk_combo_box_text_new();
    up_bun = gtk_button_new_with_label(_("上移"));
    down_bun = gtk_button_new_with_label(_("下移"));
    gtk_widget_set_sensitive(up_bun, FALSE);
    gtk_widget_set_sensitive(down_bun, FALSE);
    
    hbox_cd = gtk_hbox_new(FALSE, 10);
    hbox_lead = gtk_hbox_new(FALSE, 10);
    vbox_bun = gtk_vbox_new(FALSE, 3);

    gtk_box_pack_start(GTK_BOX(vbox_bun), up_bun, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(vbox_bun), down_bun, TRUE, TRUE, 2);

    
    gtk_box_pack_start(GTK_BOX(hbox_cd), cd, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_cd), cd1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_lead), lead, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_lead), lead1, TRUE, TRUE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_lead), vbox_bun, FALSE, FALSE, 5);

    if(NULL != isos)
    {
        for(int i=0; i<cJSON_GetArraySize(isos); i++)
        {
            iso = cJSON_GetArrayItem(isos, i)->valuestring;
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cd1), iso);
        }
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(cd1), 0);

    seq = gtk_hseparator_new();
    dialog = gtk_dialog_new_with_buttons(_("运行一次"), GTK_WINDOW(win->toplevel),
                                    GTK_DIALOG_MODAL,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_OK, GTK_RESPONSE_OK,
                                    NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
    gtk_widget_set_size_request(dialog, 400, 300);  
    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(area), hbox_cd, FALSE, FALSE, 20);
    gtk_box_pack_start(GTK_BOX(area), hbox_lead, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(area), seq, TRUE, TRUE, 5);

    g_signal_connect(up_bun, "clicked", G_CALLBACK(up_current_lead), lead1);
    g_signal_connect(down_bun, "clicked", G_CALLBACK(down_current_lead), lead1);
    
    gtk_widget_show_all(dialog);

    int i=1;
    char *c_lead, index[6], value[10];
    switch(gtk_dialog_run(GTK_DIALOG(dialog)))
    {
        case GTK_RESPONSE_OK:
            result = cJSON_CreateObject();
            
            iso = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(cd1));
            if(NULL == iso)
                cJSON_AddStringToObject(result, "iso", "");
            else
                cJSON_AddStringToObject(result, "iso", iso);
            model = GTK_TREE_MODEL(store);
            gtk_tree_model_get_iter_first(model, &iter); 
            do
            {
                switch(i)
                {
                    case 1:
                        strcpy(index, "lead1");
                        break;
                    case 2:
                        strcpy(index, "lead2");
                        break;
                    case 3:
                        strcpy(index, "lead3");
                        break;
                    default:
                        break;
                }

                gtk_tree_model_get(model, &iter, 0, &c_lead, -1);
                if(strstr(c_lead, "硬盘"))
                    strcpy(value, "disk");
                if(strstr(c_lead, "CD"))
                    strcpy(value, "cdrom");
                if(strstr(c_lead, "PXE"))
                    strcpy(value, "interface");
                cJSON_AddStringToObject(result, index, value);
                i++;
                g_free(c_lead);
            }while(gtk_tree_model_iter_next(model, &iter));

            g_free(iso);
            break;
            
        default:
            break;
    }

    gtk_widget_destroy(dialog);
    gtk_tree_store_clear(store);
    if(current_lead)
    {
        g_free(current_lead);
        current_lead = NULL;
    }
    return result;
}
static char* dialog_for_changeCd(cJSON *data)
{
    GtkWidget *dialog, *area, *seq;
    GtkWidget *label, *cds;
    cJSON *isos=data;
    char *cd, *iso=NULL;

    label = gtk_label_new(_("切换CD:"));
    cds = gtk_combo_box_text_new();
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    
    if(NULL != isos)
    {
        for(int i=0; i<cJSON_GetArraySize(isos); i++)
        {
            cd = cJSON_GetArrayItem(isos, i)->valuestring;
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cds), cd);
        }
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(cds), 0);

    dialog = gtk_dialog_new_with_buttons(_("切换ISO镜像:"), GTK_WINDOW(win->toplevel),
                                    GTK_DIALOG_MODAL,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_OK, GTK_RESPONSE_OK,
                                    NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
    gtk_widget_set_size_request(dialog, 400, 250);  
    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    seq = gtk_hseparator_new();
        
    gtk_box_pack_start(GTK_BOX(area), label, FALSE, FALSE, 20);
    gtk_box_pack_start(GTK_BOX(area), cds, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(area), seq, TRUE, TRUE, 0);

    gtk_widget_show_all(dialog);
    switch(gtk_dialog_run(GTK_DIALOG(dialog)))
    {
        case GTK_RESPONSE_OK:
            iso = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(cds));
            break ;
        default:
            break;
    }

    gtk_widget_destroy(dialog);
    return iso;
}
static void menu_cb_CreateImage(GtkButton *button, gpointer *data)
{
    char *host;
    cJSON *disk, *domains, *recvInfo;
    char *name, *notes, *size, *storage, *policy, *c_status;
    GtkWidget *image = (GtkWidget *)data;
    int status;
    
    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry)); 
    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/dataDomain");
    
    pthread_mutex_unlock(&lock);
    GThread *td = g_thread_new ("get_data_domain", curl_send_get, "get_data_domain");
    g_thread_join(td);

    pthread_mutex_trylock(&lock);
        
    recvInfo = cjson_recv;
    if(NULL == recvInfo)
    {
        g_free(host);
        dialog_for_message(_("获取存储域失败\n不能创建新磁盘!"));
        pthread_mutex_unlock(&lock);
        return ;
    }
    c_status = cJSON_PrintUnformatted(cJSON_GetObjectItem(recvInfo, "status"));
    if(NULL != c_status)
    {
        status = atoi(c_status);
        free(c_status);
        if(-6 == status)
            dialog_for_message(_("登录异常\n连接已断开!"));
        else
        {
            char *error = get_cjson_data(recvInfo, "message");
            dialog_for_message(error);    
        }
        g_free(host);
        cJSON_Delete(recvInfo);
        pthread_mutex_unlock(&lock);
        return ;
    }
    domains = cJSON_GetObjectItem(recvInfo, "dataStorageList");
    if(0 == cJSON_GetArraySize(domains))
    {
        g_free(host);
        cJSON_Delete(recvInfo);
        dialog_for_message(_("未获取到活动的存储域\n不能新建磁盘!"));
        pthread_mutex_unlock(&lock);
        return ;
    }
    disk = dialog_for_newImage(domains);
    cJSON_Delete(recvInfo);
    if(NULL == disk)
    {
        g_free(host);
        pthread_mutex_unlock(&lock); 
        return ;
    }

    name = get_cjson_data(disk, "name");
    notes = get_cjson_data(disk, "notes");
    size = get_cjson_data(disk, "size");
    storage = get_cjson_data(disk, "storage");
    policy = get_cjson_data(disk, "policy");

    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/createvmdisk");
    
    params = g_string_new("diskAlias=");
    g_string_append(params, name);
    g_string_append(params, "&description=");
    if(NULL == notes)
        g_string_append(params, "");
    else
        g_string_append(params, notes);
    g_string_append(params, "&size=");
    g_string_append(params, size);
    g_string_append(params, "&domainName=");
    g_string_append(params, storage);
    g_string_append(params, "&VolType=");
    g_string_append(params, policy);

    pthread_mutex_unlock(&lock);
    GThread *td1 = g_thread_new ("post_new_image", curl_send_post, "post_new_image");
    dialog_for_progress(_("正在创建磁盘:"));
    g_thread_join(td1);

    pthread_mutex_trylock(&lock);
    recvInfo = cjson_recv;
    if(NULL == recvInfo)
    {
        g_free(host);
        cJSON_Delete(disk);
        dialog_for_message(_("创建磁盘失败!"));
        pthread_mutex_unlock(&lock);
        return ;
    }
    
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        gtk_entry_set_text(GTK_ENTRY(image), name); 
        strcpy(new_vm_imgId, get_cjson_data(recvInfo, "imageUUID"));

    } else {

        if(-6 == status)
            dialog_for_message(_("登录异常\n连接已断开!"));
        else
        {
            char *error = get_cjson_data(recvInfo, "message");
            dialog_for_message(error);
        }
    }

    g_free(host);
    cJSON_Delete(disk);
    cJSON_Delete(recvInfo);
    pthread_mutex_unlock(&lock);
    return ;
}
static void menu_cb_AttachImage(GtkButton *button, gpointer *data)
{
    char *host;
    cJSON *recvInfo, *disks;
    GtkWidget *image = (GtkWidget *)data;
    int status;
    
    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry)); 
    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/unattachimage");
    
    pthread_mutex_unlock(&lock);
    GThread *td = g_thread_new ("get_unattach_image", curl_send_get, "get_unattach_image");
    dialog_for_progress(_("正在获取未附加的磁盘"));
    g_thread_join(td);

    pthread_mutex_trylock(&lock);
        
    recvInfo = cjson_recv;
    if(NULL == recvInfo)
    {
        g_free(host);
        dialog_for_message(_("获取未附加磁盘失败!"));
        pthread_mutex_unlock(&lock);
        return ;
    }
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 != status)
    {   
        if(-6 == status)
            dialog_for_message(_("登录异常\n连接已断开!"));
        else   
        {    
            char *error = get_cjson_data(recvInfo, "message");
            dialog_for_message(error);
        }
        g_free(host);
        cJSON_Delete(recvInfo);
        pthread_mutex_unlock(&lock);
        return ;
    }
    
    disks = cJSON_GetObjectItem(recvInfo, "diskList");
    dialog_for_attachImage(disks, image);

    g_free(host);
    cJSON_Delete(recvInfo);
    pthread_mutex_unlock(&lock);
}

static void menu_cb_memsize_out(GtkWidget *entry, GdkEvent  *event, gpointer *data)
{
    GtkWidget *maxsize=(GtkWidget *)data;
    const char *size, *max;
    char value[20];
    int mem_size, max_size;

    size = gtk_entry_get_text(GTK_ENTRY(entry));
    max = gtk_entry_get_text(GTK_ENTRY(maxsize));
    mem_size = atoi(size);
    max_size = atoi(max);

    sprintf(value, "%d", mem_size);
    if(max_size < mem_size)
        gtk_entry_set_text(GTK_ENTRY(maxsize), value);
}
static void menu_cb_maxmem_in(GtkWidget *entry, GdkEvent  *event, gpointer *data)
{
    GtkWidget *memsize=(GtkWidget *)data;
    const char *size;
    char value[20];
    int mem_size;

    size = gtk_entry_get_text(GTK_ENTRY(memsize));
    mem_size = atoi(size);

    sprintf(value, "%d", 4*mem_size);
    gtk_entry_set_text(GTK_ENTRY(entry), value);
}
static void menu_cb_maxmem_out(GtkWidget *entry, GdkEvent  *event, gpointer *data)
{
    GtkWidget *memsize=(GtkWidget *)data;
    const char *size, *max;
    char value[20];
    int mem_size, max_size;

    size = gtk_entry_get_text(GTK_ENTRY(memsize));
    max = gtk_entry_get_text(GTK_ENTRY(entry));
    mem_size = atoi(size);
    max_size = atoi(max);

    sprintf(value, "%d", mem_size);
    if(max_size < mem_size)
        gtk_entry_set_text(GTK_ENTRY(entry), value);
}
static cJSON* dialog_for_newVm_temp(void *data)
{
    GtkWidget *dialog, *area, *seq;
    cJSON *result = NULL, *temps=data, *tmp;
    GtkWidget *center, *temp, *number, *name, *notes, *memsize, *maxmem, *cpu;
    GtkWidget *center1, *temp1, *number1, *name1, *notes1, *memsize1, *maxmem1, *cpu1;
    GtkWidget *hbox_center, *hbox_temp, *hbox_number, *hbox_name, *hbox_notes,
                                            *hbox_memsize, *hbox_maxmem, *hbox_cpu;
    const char *center2, *number2, *name2, *notes2, *memsize2, *maxmem2, *cpu2;
    char *temp_name, *temp2, *tempId=NULL;
    
    center = gtk_label_new(_("数   据   中   心"));
    temp = gtk_label_new(_("使   用   模   板"));
    number = gtk_label_new(_("虚 拟 机 个 数"));
    name = gtk_label_new(_("名                   称"));
    notes = gtk_label_new(_("注                   释"));
    memsize = gtk_label_new(_("内        存(MB)"));
    maxmem = gtk_label_new(_("最大内存(MB)"));
    cpu = gtk_label_new(_("虚拟CPU总数"));
    
    center1 = gtk_entry_new();
    temp1 = gtk_combo_box_text_new();
    number1 = gtk_entry_new();
    name1 = gtk_entry_new();
    notes1 = gtk_entry_new();
    memsize1 = gtk_entry_new();
    maxmem1 = gtk_entry_new();
    cpu1 = gtk_entry_new();

    hbox_center = gtk_hbox_new(FALSE, 2); 
    hbox_temp = gtk_hbox_new(FALSE, 2);
    hbox_number = gtk_hbox_new(FALSE, 2);
    hbox_name = gtk_hbox_new(FALSE, 2);
    hbox_notes = gtk_hbox_new(FALSE, 2);
    hbox_memsize = gtk_hbox_new(FALSE, 2);
    hbox_maxmem = gtk_hbox_new(FALSE, 2);
    hbox_cpu = gtk_hbox_new(FALSE, 2);

    gtk_box_pack_start(GTK_BOX(hbox_center), center, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_center), center1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_temp), temp, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_temp), temp1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_number), number, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_number), number1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_name), name, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_name), name1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_notes), notes, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_notes), notes1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_memsize), memsize, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_memsize), memsize1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_maxmem), maxmem, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_maxmem), maxmem1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_cpu), cpu, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_cpu), cpu1, TRUE, TRUE, 5);

    for(int i=0; i<cJSON_GetArraySize(temps); i++)
    {
        tmp = cJSON_GetArrayItem(temps, i);
        temp_name = get_cjson_data(tmp, "name");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(temp1), temp_name);
    }
    gtk_widget_set_sensitive(center1, FALSE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(temp1), 0);
    gtk_entry_set_text(GTK_ENTRY(center1), _("default"));
    gtk_entry_set_max_length(GTK_ENTRY(number1), 3);
    gtk_entry_set_max_length(GTK_ENTRY(name1), 32);
    gtk_entry_set_max_length(GTK_ENTRY(notes1), 128);
    gtk_entry_set_max_length(GTK_ENTRY(memsize1), 10);
    gtk_entry_set_max_length(GTK_ENTRY(maxmem1), 12);
    gtk_entry_set_max_length(GTK_ENTRY(cpu1), 2);
    
    g_signal_connect(memsize1, "focus-out-event", G_CALLBACK(menu_cb_memsize_out), maxmem1);
    g_signal_connect(maxmem1, "focus-in-event", G_CALLBACK(menu_cb_maxmem_in), memsize1);
    g_signal_connect(maxmem1, "focus-out-event", G_CALLBACK(menu_cb_maxmem_out), memsize1);
    g_signal_connect(G_OBJECT(number1), "insert-text", G_CALLBACK(on_entry_insert_text), NULL);
    g_signal_connect(G_OBJECT(memsize1), "insert-text", G_CALLBACK(on_entry_insert_text), NULL);
    g_signal_connect(G_OBJECT(maxmem1), "insert-text", G_CALLBACK(on_entry_insert_text), NULL);
    g_signal_connect(G_OBJECT(cpu1), "insert-text", G_CALLBACK(on_entry_insert_text), NULL);
    
    seq = gtk_hseparator_new();
    dialog = gtk_dialog_new_with_buttons(_("新建虚拟机"), GTK_WINDOW(win->toplevel),
                                    GTK_DIALOG_MODAL,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_OK, GTK_RESPONSE_OK,
                                    NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
    gtk_widget_set_size_request(dialog, 500, 450);
        
    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(area), hbox_center, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_temp, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_number, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_name, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_notes, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_memsize, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_maxmem, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_cpu, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), seq, TRUE, TRUE, 10);
    
    gtk_widget_show_all(dialog); 

    int flag;
    while( TRUE )
    {
        flag = 1;
        switch(gtk_dialog_run(GTK_DIALOG(dialog)))
        {
            case GTK_RESPONSE_OK:
                center2 = gtk_entry_get_text(GTK_ENTRY(center1));
                temp2 = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(temp1));
                number2 = gtk_entry_get_text(GTK_ENTRY(number1));
                name2 = gtk_entry_get_text(GTK_ENTRY(name1));
                notes2 = gtk_entry_get_text(GTK_ENTRY(notes1));
                memsize2 = gtk_entry_get_text(GTK_ENTRY(memsize1));
                maxmem2 = gtk_entry_get_text(GTK_ENTRY(maxmem1));
                cpu2 = gtk_entry_get_text(GTK_ENTRY(cpu1));

                if(0 == g_strcmp0(name2, "") || 0 == g_strcmp0(memsize2, "") 
                       || 0 == g_strcmp0(maxmem2, "") || 0 == g_strcmp0(number2, "") 
                            || 0 == g_strcmp0(cpu2, ""))
                {
                    dialog_for_message(_("除注释外，其他项不能为空\n请检查!"));
                    break ;
                }
                result = cJSON_CreateObject();
                cJSON_AddStringToObject(result, "center", center2);
                for(int j=0; j<cJSON_GetArraySize(temps); j++)
                {
                    tmp = cJSON_GetArrayItem(temps, j);
                    temp_name = get_cjson_data(tmp, "name");
                    if(0 == g_strcmp0(temp2, temp_name))
                    {    
                        tempId = get_cjson_data(tmp, "tempUUID");
                        cJSON_AddStringToObject(result, "temp", tempId);
                        break ;
                    }
                }  
                cJSON_AddStringToObject(result, "number", number2);
                cJSON_AddStringToObject(result, "name", name2);
                cJSON_AddStringToObject(result, "notes", notes2);
                cJSON_AddStringToObject(result, "memsize", memsize2);
                cJSON_AddStringToObject(result, "maxmem", maxmem2);
                cJSON_AddStringToObject(result, "cpu", cpu2);
                
                flag = 0;
                g_free(temp2);
                break ;       
            default:
                flag = 0;
                break ;
        }
        if(0 == flag)
            break ;
    }
    gtk_widget_destroy(dialog);
    return result;
}
static cJSON* dialog_for_newVm(void)
{
    GtkWidget *dialog, *area, *seq;
    cJSON *result = NULL;
    
    GtkWidget *center, *name, *notes, *memsize, *maxmem, *cpu, *image;
    GtkWidget *center1, *name1, *notes1, *memsize1, *maxmem1, *cpu1, *image1;
    GtkWidget *label, *label1, *add_img, *new_img;
    GtkWidget *net, *protocol, *video;
    GtkWidget *net1, *protocol1, *video1;
    GtkWidget *hbox_center, *hbox_name, *hbox_notes, *hbox_memsize, *hbox_maxmem;
    GtkWidget *hbox_cpu, *hbox_image, *hbox_net, *hbox_protocol, *hbox_video;
    const char *c_center, *c_name, *c_notes, *c_memsize, *c_maxmem, *c_cpu, *c_image;
    char *c_net, *c_protocol, *c_video;
    
    center = gtk_label_new(_("数   据   中   心"));
    name = gtk_label_new(_("名                   称"));
    notes = gtk_label_new(_("注                   释"));
    memsize = gtk_label_new(_("内        存(MB)"));
    maxmem = gtk_label_new(_("最大内存(MB)"));
    cpu = gtk_label_new(_("虚拟CPU总数"));
    image = gtk_label_new(_("实例磁盘镜像")); 
    label = gtk_label_new(_("vNIC配置实例化虚拟网络接口:"));
    net = gtk_label_new(_("网   络   接   口"));
    label1 = gtk_label_new(_("图形控制台:"));
    protocol = gtk_label_new(_("图形界面协议"));
    video = gtk_label_new(_("视   频   类   型"));
    add_img = gtk_button_new_with_label("附 加");
    new_img = gtk_button_new_with_label("新 建");
    
    center1 = gtk_entry_new();
    name1 = gtk_entry_new();
    notes1 = gtk_entry_new();
    memsize1 = gtk_entry_new();
    maxmem1 = gtk_entry_new();
    cpu1 = gtk_entry_new();
    image1 = gtk_entry_new();
    net1 = gtk_combo_box_text_new();
    protocol1 = gtk_combo_box_text_new();
    video1 = gtk_combo_box_text_new();
    
    hbox_center = gtk_hbox_new(FALSE, 2); 
    hbox_name = gtk_hbox_new(FALSE, 2);
    hbox_notes = gtk_hbox_new(FALSE, 2);
    hbox_memsize = gtk_hbox_new(FALSE, 2);
    hbox_maxmem = gtk_hbox_new(FALSE, 2);
    hbox_cpu = gtk_hbox_new(FALSE, 2);
    hbox_image = gtk_hbox_new(FALSE, 2);
    hbox_net = gtk_hbox_new(FALSE, 2);
    hbox_protocol = gtk_hbox_new(FALSE, 2);
    hbox_video = gtk_hbox_new(FALSE, 2);

    gtk_box_pack_start(GTK_BOX(hbox_center), center, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_center), center1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_name), name, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_name), name1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_notes), notes, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_notes), notes1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_memsize), memsize, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_memsize), memsize1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_maxmem), maxmem, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_maxmem), maxmem1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_cpu), cpu, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_cpu), cpu1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_image), image, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_image), image1, TRUE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox_image), add_img, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox_image), new_img, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_net), net, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_net), net1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_protocol), protocol, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_protocol), protocol1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_video), video, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_video), video1, TRUE, TRUE, 5);

    gtk_entry_set_text(GTK_ENTRY(center1), _("default"));
    gtk_entry_set_max_length(GTK_ENTRY(name1), 32);
    gtk_entry_set_max_length(GTK_ENTRY(notes1), 128);
    gtk_entry_set_max_length(GTK_ENTRY(memsize1), 10);
    gtk_entry_set_max_length(GTK_ENTRY(maxmem1), 12);
    gtk_entry_set_max_length(GTK_ENTRY(cpu1), 2);
    gtk_widget_set_sensitive(center1, FALSE);
    gtk_widget_set_sensitive(image1, FALSE);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(net1), _("ovirtmgmt"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(protocol1), _("spice"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(protocol1), _("VNC"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(video1), _("qxl"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(video1), _("vga"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(video1), _("cirrus"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(net1), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(protocol1), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(video1), 0);

    g_signal_connect(new_img, "clicked", G_CALLBACK(menu_cb_CreateImage), image1);
    g_signal_connect(add_img, "clicked", G_CALLBACK(menu_cb_AttachImage), image1);
    g_signal_connect(memsize1, "focus-out-event", G_CALLBACK(menu_cb_memsize_out), maxmem1);
    g_signal_connect(maxmem1, "focus-in-event", G_CALLBACK(menu_cb_maxmem_in), memsize1);
    g_signal_connect(maxmem1, "focus-out-event", G_CALLBACK(menu_cb_maxmem_out), memsize1);
    g_signal_connect(G_OBJECT(memsize1), "insert-text", G_CALLBACK(on_entry_insert_text), NULL);
    g_signal_connect(G_OBJECT(maxmem1), "insert-text", G_CALLBACK(on_entry_insert_text), NULL);
    g_signal_connect(G_OBJECT(cpu1), "insert-text", G_CALLBACK(on_entry_insert_text), NULL);
    
    seq = gtk_hseparator_new();
    dialog = gtk_dialog_new_with_buttons(_("新建虚拟机"), GTK_WINDOW(win->toplevel),
                                    GTK_DIALOG_MODAL,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_OK, GTK_RESPONSE_OK,
                                    NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
    gtk_widget_set_size_request(dialog, 550, 600);
        
    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(area), hbox_center, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_name, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_notes, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_memsize, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_maxmem, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_cpu, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_image, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_net, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), label1, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_protocol, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_video, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), seq, TRUE, TRUE, 10);
    
    gtk_widget_show_all(dialog); 

    int flag;
    while( TRUE )
    {
        flag = 1;
        switch(gtk_dialog_run(GTK_DIALOG(dialog)))
        {
            case GTK_RESPONSE_OK:
                c_center = gtk_entry_get_text(GTK_ENTRY(center1));
                c_name = gtk_entry_get_text(GTK_ENTRY(name1));
                c_notes = gtk_entry_get_text(GTK_ENTRY(notes1));
                c_memsize = gtk_entry_get_text(GTK_ENTRY(memsize1));
                c_maxmem = gtk_entry_get_text(GTK_ENTRY(maxmem1));
                c_cpu = gtk_entry_get_text(GTK_ENTRY(cpu1));
                c_image = gtk_entry_get_text(GTK_ENTRY(image1));
                c_net = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(net1));
                c_protocol = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(protocol1));
                c_video = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(video1));

                if(0 == g_strcmp0(c_name, "") || 0 == g_strcmp0(c_memsize, "") || 0 == g_strcmp0(c_maxmem, "") || 
                                                    0 == g_strcmp0(c_cpu, "")|| 0 == g_strcmp0(c_image, ""))
                {
                    dialog_for_message(_("除注释外，其他项不能为空\n请检查!"));
                    break ;
                }
                result = cJSON_CreateObject();
                cJSON_AddStringToObject(result, "center", c_center);
                cJSON_AddStringToObject(result, "name", c_name);
                cJSON_AddStringToObject(result, "notes", c_notes);
                cJSON_AddStringToObject(result, "memsize", c_memsize);
                cJSON_AddStringToObject(result, "maxmem", c_maxmem);
                cJSON_AddStringToObject(result, "cpu", c_cpu);
                cJSON_AddStringToObject(result, "image", c_image);
                cJSON_AddStringToObject(result, "net", c_net);
                cJSON_AddStringToObject(result, "protocol", c_protocol);
                cJSON_AddStringToObject(result, "video", c_video);
                
                flag = 0;
                g_free(c_net);
                g_free(c_protocol);
                g_free(c_video);
                break ;       
            default:
                flag = 0;
                break ;
        }
        if(0 == flag)
            break ;
    }
    gtk_widget_destroy(dialog);
    return result;
}
static cJSON* dialog_for_editVm(void *data)
{
    GtkWidget *dialog, *area, *seq;
    cJSON *vm=data, *result = NULL;
    
    GtkWidget *name, *notes, *mac, *memsize, *maxmem, *cpu, *c_cpu, *t_cpu;
    GtkWidget *name1, *notes1, *mac1, *memsize1, *maxmem1, *cpu1, *c_cpu1, *t_cpu1;
    GtkWidget *hbox_name, *hbox_notes, *hbox_mac, *hbox_memsize, 
                                *hbox_maxmem, *hbox_cpu, *hbox_c_cpu, *hbox_t_cpu;
    const char *name2, *notes2, *mac2, *memsize2, *maxmem2, *cpu2, *c_cpu2, *t_cpu2;
    
    name = gtk_label_new(_("名                   称"));
    notes = gtk_label_new(_("注                   释"));
    mac = gtk_label_new(_("MAC    地    址"));
    memsize = gtk_label_new(_("内         存(MB)"));
    maxmem = gtk_label_new(_("最大内存(MB)"));
    cpu = gtk_label_new(_("虚拟CPU总数"));
    c_cpu = gtk_label_new(_("单个CPU内核数"));
    t_cpu = gtk_label_new(_("每个内核线程数"));

    name1 = gtk_entry_new();
    notes1 = gtk_entry_new();
    mac1 = gtk_entry_new();
    memsize1 = gtk_entry_new();
    maxmem1 = gtk_entry_new();
    cpu1 = gtk_entry_new();
    c_cpu1 = gtk_entry_new();
    t_cpu1 = gtk_entry_new();
    
    hbox_name = gtk_hbox_new(FALSE, 2);
    hbox_notes = gtk_hbox_new(FALSE, 2);
    hbox_mac = gtk_hbox_new(FALSE, 2); 
    hbox_memsize = gtk_hbox_new(FALSE, 2);
    hbox_maxmem = gtk_hbox_new(FALSE, 2);
    hbox_cpu = gtk_hbox_new(FALSE, 2);
    hbox_c_cpu = gtk_hbox_new(FALSE, 2);
    hbox_t_cpu = gtk_hbox_new(FALSE, 2);

    if(vm != NULL)
    {
        char *vmName = get_cjson_data(vm, "vmName");
        gtk_entry_set_text(GTK_ENTRY(name1), vmName);
        gtk_entry_set_text(GTK_ENTRY(notes1), get_vm_property(vmName, "description"));
        gtk_entry_set_text(GTK_ENTRY(mac1), get_cjson_data(vm, "macAddr"));
        gtk_entry_set_text(GTK_ENTRY(memsize1), get_cjson_data(vm, "memSize"));
        gtk_entry_set_text(GTK_ENTRY(cpu1), get_cjson_data(vm, "cpuNum"));
        gtk_entry_set_text(GTK_ENTRY(c_cpu1), get_cjson_data(vm, "coresPerCpu"));
        gtk_entry_set_text(GTK_ENTRY(t_cpu1), get_cjson_data(vm, "threadsPerCore"));
    }

    gtk_box_pack_start(GTK_BOX(hbox_name), name, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_name), name1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_notes), notes, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_notes), notes1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_mac), mac, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_mac), mac1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_memsize), memsize, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_memsize), memsize1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_maxmem), maxmem, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_maxmem), maxmem1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_cpu), cpu, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_cpu), cpu1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_c_cpu), c_cpu, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_c_cpu), c_cpu1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_t_cpu), t_cpu, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(hbox_t_cpu), t_cpu1, TRUE, TRUE, 5);

    gtk_entry_set_max_length(GTK_ENTRY(name1), 32);
    gtk_entry_set_max_length(GTK_ENTRY(notes1), 128);
    gtk_entry_set_max_length(GTK_ENTRY(mac1), 17);
    gtk_entry_set_max_length(GTK_ENTRY(memsize1), 10);
    gtk_entry_set_max_length(GTK_ENTRY(maxmem1), 12);
    gtk_entry_set_max_length(GTK_ENTRY(cpu1), 2);
    gtk_entry_set_max_length(GTK_ENTRY(c_cpu1), 2);
    gtk_entry_set_max_length(GTK_ENTRY(t_cpu1), 2);
    g_signal_connect(memsize1, "focus-out-event", G_CALLBACK(menu_cb_memsize_out), maxmem1);
    g_signal_connect(maxmem1, "focus-in-event", G_CALLBACK(menu_cb_maxmem_in), memsize1);
    g_signal_connect(maxmem1, "focus-out-event", G_CALLBACK(menu_cb_maxmem_out), memsize1);
    g_signal_connect(G_OBJECT(memsize1), "insert-text", G_CALLBACK(on_entry_insert_text), NULL);
    g_signal_connect(G_OBJECT(maxmem1), "insert-text", G_CALLBACK(on_entry_insert_text), NULL);
    g_signal_connect(G_OBJECT(cpu1), "insert-text", G_CALLBACK(on_entry_insert_text), NULL);
    g_signal_connect(G_OBJECT(c_cpu1), "insert-text", G_CALLBACK(on_entry_insert_text), NULL);
    g_signal_connect(G_OBJECT(t_cpu1), "insert-text", G_CALLBACK(on_entry_insert_text), NULL);

    g_signal_connect(G_OBJECT(mac1), "insert-text", G_CALLBACK(on_mac_entry_insert_text), NULL);
    
    seq = gtk_hseparator_new();
    dialog = gtk_dialog_new_with_buttons(_("编辑虚拟机"), GTK_WINDOW(win->toplevel),
                                    GTK_DIALOG_MODAL,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_OK, GTK_RESPONSE_OK,
                                    NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
    gtk_widget_set_size_request(dialog, 480, 420);
        
    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(area), hbox_name, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_notes, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_mac, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_memsize, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_maxmem, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_cpu, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_c_cpu, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox_t_cpu, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), seq, TRUE, TRUE, 10);
    
    gtk_widget_show_all(dialog); 

    int flag;
    while( TRUE )
    {
        flag = 1;
        switch(gtk_dialog_run(GTK_DIALOG(dialog)))
        {
            case GTK_RESPONSE_OK:
                name2 = gtk_entry_get_text(GTK_ENTRY(name1));
                notes2 = gtk_entry_get_text(GTK_ENTRY(notes1));
                mac2 = gtk_entry_get_text(GTK_ENTRY(mac1));
                memsize2 = gtk_entry_get_text(GTK_ENTRY(memsize1));
                maxmem2 = gtk_entry_get_text(GTK_ENTRY(maxmem1));
                cpu2 = gtk_entry_get_text(GTK_ENTRY(cpu1));
                c_cpu2 = gtk_entry_get_text(GTK_ENTRY(c_cpu1));
                t_cpu2 = gtk_entry_get_text(GTK_ENTRY(t_cpu1));
                    
                if(0 == g_strcmp0(name2, "") || 0 == g_strcmp0(mac2, "") || 0 == g_strcmp0(memsize2, "") ||
                    0 == g_strcmp0(maxmem2, "") ||  0 == g_strcmp0(cpu2, "") ||  0 == g_strcmp0(c_cpu2, "") 
                            ||  0 == g_strcmp0(t_cpu2, ""))
                {
                    dialog_for_message(_("除注释外，其他项不能为空\n请检查!"));
                    break ;
                }
                result = cJSON_CreateObject();
                cJSON_AddStringToObject(result, "name", name2);
                cJSON_AddStringToObject(result, "notes", notes2);
                cJSON_AddStringToObject(result, "mac", mac2);
                cJSON_AddStringToObject(result, "memsize", memsize2);
                cJSON_AddStringToObject(result, "maxmem", maxmem2);
                cJSON_AddStringToObject(result, "cpu", cpu2);
                cJSON_AddStringToObject(result, "c_cpu", c_cpu2);
                cJSON_AddStringToObject(result, "t_cpu", t_cpu2);
                
                flag = 0;
                break ;       
            default:
                flag = 0;
                break ;
        }
        if(0 == flag)
            break ;
    }
    gtk_widget_destroy(dialog);
    return result;
}
static void menu_cb_CreateVm(GtkToolButton *toolbutton, void *data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return ;
    }
    g_source_remove(timer);
    
    char *center, *temp, *number, *name, *notes, *memsize, *maxmem;
    char *cpu, *image, *net, *protocol, *video;
    char *host;
    int status;
    cJSON *recvInfo, *vmInfo, *temps;

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    if(dialog_for_confirm(_("是否使用模板?"), NULL))
    {
        url = g_string_new("https://");
        g_string_append(url, host);
        g_string_append(url, ":8443/vm/getalltemps?");
        g_string_append(url, "&_search=false");
        g_string_append(url, "&nd=1514358456936");
        g_string_append(url, "&rows=100");
        g_string_append(url, "&page=1");
        g_string_append(url, "&sidx=id");
        g_string_append(url, "&sord=desc");

        pthread_mutex_unlock(&lock);
        GThread *td = g_thread_new("get_temps", curl_send_get, "get_temps");
        g_thread_join(td);
        recvInfo = cjson_recv;
        if(NULL == recvInfo)
        {
            g_free(host);
            dialog_for_message(_("获取模板信息失败!"));
            timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
            pthread_mutex_unlock(&lock);
            return ;
        }
        status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
        if(0 != status)
        {
            g_free(host);
            cJSON_Delete(recvInfo);
            if(-6 == status)
                dialog_for_message(_("登录异常\n连接已断开!"));
            else
            {
                dialog_for_message(_("获取模板信息失败!"));
                timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
            }
            pthread_mutex_unlock(&lock);
            return ;     
        }
        temps = cJSON_GetObjectItem(recvInfo, "rows");
        if(0 == cJSON_GetArraySize(temps))
        {
            g_free(host);
            cJSON_Delete(recvInfo);
            dialog_for_message(_("未获取到模板信息\n请选择其他创建方式!"));
            timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
            pthread_mutex_unlock(&lock);
            return ;
        }
        vmInfo = dialog_for_newVm_temp(temps);
        cJSON_Delete(recvInfo);
    }
    else
        vmInfo = dialog_for_newVm();
    
    if(NULL == vmInfo)
    {
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        g_free(host);
        pthread_mutex_unlock(&lock);
        return ;
    }    
    center = get_cjson_data(vmInfo, "center");
    name = get_cjson_data(vmInfo, "name");
    strcpy(current_conn_vm, name);
    notes = get_cjson_data(vmInfo, "notes");
    memsize = get_cjson_data(vmInfo, "memsize");
    maxmem = get_cjson_data(vmInfo, "maxmem");
    cpu = get_cjson_data(vmInfo, "cpu");
    
    if(NULL != (temp=get_cjson_data(vmInfo, "temp")))
        number = get_cjson_data(vmInfo, "number");
    else
    {
        image=new_vm_imgId;
        net = get_cjson_data(vmInfo, "net");
        protocol = get_cjson_data(vmInfo, "protocol");
        video = get_cjson_data(vmInfo, "video");
    }
      
    url = g_string_new("https://");
    g_string_append(url, host);
    if(NULL != temp)
    {
        g_string_append(url, ":8443/vm/createvmsbytemp");
        params = g_string_new("name=");
    }
    else
    {
        g_string_append(url, ":8443/vm/createvm");
        params = g_string_new("vmName=");
    }
    g_string_append(params, name);
    g_string_append(params, "&dataCenter=");
    g_string_append(params, center);
    g_string_append(params, "&description=");
    g_string_append(params, notes);
    g_string_append(params, "&memSize=");
    g_string_append(params, memsize);
    g_string_append(params, "&maxMemSize=");
    g_string_append(params, maxmem);
    g_string_append(params, "&smp=");
    g_string_append(params, cpu);
    g_string_append(params, "&smpCoresPerSocket=");
    g_string_append(params, "1");
    g_string_append(params, "&smpThreadsPerCore=");
    g_string_append(params, "1");
    if(NULL != temp)
    {
        g_string_append(params, "&tempUUID=");
        g_string_append(params, temp);
        g_string_append(params, "&num=");
        g_string_append(params, number);
    } else {
        
        g_string_append(params, "&acpiEnable=");
        g_string_append(params, "true");
        g_string_append(params, "&displayNetwork=");
        g_string_append(params, net);
        g_string_append(params, "&imageUUID=");
        g_string_append(params, image);
        g_string_append(params, "&graphics=");
        g_string_append(params, protocol);
        g_string_append(params, "&video=");
        g_string_append(params, video);
    }

    pthread_mutex_unlock(&lock);
    g_thread_new("post_new_vm", curl_send_post, "post_new_vm");

    dialog_for_progress(_("正在创建虚拟机:"));
    g_free(host);
    cJSON_Delete(vmInfo);
}
static void createVm(void *data)
{
    cJSON *recvInfo = data;
    int status; 

    close_progress_dialog(NULL);
    if(NULL == recvInfo)
    {
        dialog_for_message("返回信息异常，解析失败!");
        return ;
    }

    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        pthread_mutex_unlock(&lock);
        get_allVmInfo(NULL);
        dialog_for_message(_("虚拟机创建成功!"));
        
    } else {

        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }

    cJSON_Delete(recvInfo);
}
static void menu_cb_EditVm(GtkToolButton *toolbutton, void *data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return ;
    }
    g_source_remove(timer);

    cJSON *vm, *recvInfo;
    char *host, *vmName, *vmId;
    char *name, *notes, *mac, *memsize, *maxmem, *cpu, *c_cpu, *t_cpu;
    int index, status;
    GThread *td;

    index = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->vmbar));
    vmName = get_vmbar_vmname(index);
    strcpy(current_conn_vm, vmName);

    pthread_mutex_unlock(&lock);
    td = g_thread_new("vm_id", get_vm_info, vmName);
    g_thread_join(td);

    pthread_mutex_trylock(&lock);
    recvInfo = cjson_recv;
    if(NULL == recvInfo)
    {
        dialog_for_message("获取当前虚拟机信息失败!");
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        pthread_mutex_unlock(&lock);
        return ;
    }

    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 != status)
    {
        cJSON_Delete(recvInfo);  
        if(-6 == status)
            dialog_for_message(_("登录异常\n连接已断开!"));
        else
        {
            dialog_for_message(_("获取当前虚拟机信息失败!"));
            timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        }
        pthread_mutex_unlock(&lock);
        return ;
    }
    vm = dialog_for_editVm(cJSON_GetObjectItem(recvInfo, "vmInfo"));
    if(NULL == vm)
    {
        cJSON_Delete(recvInfo);  
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        pthread_mutex_unlock(&lock);
        return ;
    }
    vmId = get_cjson_data(cJSON_GetObjectItem(recvInfo, "vmInfo"), "vmId");
    name = get_cjson_data(vm, "name");
    notes = get_cjson_data(vm, "notes");
    mac = get_cjson_data(vm, "mac");
    memsize = get_cjson_data(vm, "memsize");
    maxmem = get_cjson_data(vm, "maxmem");
    cpu = get_cjson_data(vm, "cpu");
    c_cpu = get_cjson_data(vm, "c_cpu");
    t_cpu = get_cjson_data(vm, "t_cpu");

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/editvm");
    
    params = g_string_new("vmId=");
    g_string_append(params, vmId);
    g_string_append(params, "&vmName=");
    g_string_append(params, name);
    g_string_append(params, "&description=");
    g_string_append(params, notes);
    g_string_append(params, "&macAddr=");
    g_string_append(params, mac);
    g_string_append(params, "&memSize=");
    g_string_append(params, memsize);
    g_string_append(params, "&maxMemSize=");
    g_string_append(params, maxmem);
    g_string_append(params, "&smp=");
    g_string_append(params, cpu);
    g_string_append(params, "&smpCoresPerSocket=");
    g_string_append(params, c_cpu);
    g_string_append(params, "&smpThreadsPerCore=");
    g_string_append(params, t_cpu);

    pthread_mutex_unlock(&lock);
    g_thread_new("post_edit_vm", curl_send_post, "post_edit_vm");

    dialog_for_progress(_("正在修改虚拟机信息:"));
    g_free(host);
    cJSON_Delete(vm);
    cJSON_Delete(recvInfo);
}
static void editVm(void *data)
{
    cJSON *recvInfo = data;
    int status; 

    close_progress_dialog(NULL);
    if(NULL == recvInfo)
    {
        dialog_for_message("返回信息异常，解析失败!");
        return ;
    }

    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        pthread_mutex_unlock(&lock);
        get_allVmInfo(NULL);
        dialog_for_message(_("虚拟机信息修改成功\n部分修改重启后生效!"));
        
    } else {

        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }

    cJSON_Delete(recvInfo);
}
static int get_vm_image_type(char *vm)
{
    char *host, *vmName=vm, *img_id;
    int status, type;
    cJSON *recvInfo;

    pthread_mutex_unlock(&lock);
    GThread *td = g_thread_new("get_vm_image", get_vm_info, vmName);
    g_thread_join(td);
    pthread_mutex_trylock(&lock);
    recvInfo = cjson_recv;
    if(NULL == recvInfo)
        return 0;
    
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 != status)
    {
        cJSON_Delete(recvInfo);
        if(-6 == status)
            dialog_for_message(_("登录异常\n连接已断开!"));
        return 0;
    }
    img_id = get_cjson_data(cJSON_GetObjectItem(recvInfo, "vmInfo"), "imageUUID");
    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));

    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/storage/getdisktype?");
    g_string_append(url, "imageUUID=");
    g_string_append(url, img_id);

    g_free(host);
    cJSON_Delete(recvInfo);
    pthread_mutex_unlock(&lock);
    GThread *td1 = g_thread_new("get_img_type", curl_send_get, "get_img_type");
    g_thread_join(td1);
    pthread_mutex_trylock(&lock);
    recvInfo = cjson_recv;
    if(NULL == recvInfo)
        return 0;

    type = atoi(get_cjson_data(recvInfo, "diskType"));
    cJSON_Delete(recvInfo);
    
    return type;
}
static void menu_cb_DeleteVm(GtkToolButton *toolbutton, void *data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return ;
    }
    g_source_remove(timer);
    
    char *host, *vmName, vmId[37];
    int index, type, flag=0;

    if(!dialog_for_confirm(_("是否删除虚拟机?"), &flag))
    {
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return;
    }

    index = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->vmbar));
    vmName = get_vmbar_vmname(index);
    strcpy(vmId, get_cjson_data(vmid_list, vmName));
    strcpy(current_conn_vm, vmName);

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    if(1 == flag)
    {
        type = get_vm_image_type(vmName);   
        if(type < 1)
        {
            g_free(host);
            dialog_for_message(_("获取虚拟机磁盘类型失败\n删除虚拟机失败!"));
            timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
            pthread_mutex_unlock(&lock);
            return ;
        }

        url = g_string_new("https://");
        g_string_append(url, host);
        g_string_append(url, ":8443/vm/deletevm");
        params = g_string_new("vmId=");
        g_string_append(params, vmId);
        g_string_append(params, "&wipe=");
        if(2 == type)
           g_string_append(params, "False");
        else
           g_string_append(params, "True");
    } else {
        
        url = g_string_new("https://");
        g_string_append(url, host);
        g_string_append(url, ":8443/vm/deletevmremaindisk");
        params = g_string_new("vmId=");
        g_string_append(params, vmId);
    }

    pthread_mutex_unlock(&lock);
    g_thread_new("post_delete", curl_send_post, "post_delete");

    g_free(host);
    dialog_for_progress(_("正在删除虚拟机:"));
}
static void deleteVm_success(char *vm)
{
    char *vmName=vm;
    int index;
    conn_info *ci;
    SpiceChannel *channel;
    GtkTreeSelection *select;

    index = get_vmbar_index((char*)vmName);
    ci = get_conn_from_list_by_vmname((char*)vmName);
    
    if(NULL != ci)
    {
        channel = ci->channel;
        g_signal_handlers_block_by_func(channel, G_CALLBACK(main_channel_event), ci->conn);
        g_signal_handlers_block_by_func(channel, G_CALLBACK(main_agent_update), ci->conn);
        connection_disconnect(ci->conn);
        conn_list = g_slist_remove(conn_list, ci);
        g_free(ci->vmName);
        g_free(ci);
    } 
    gtk_notebook_remove_page(GTK_NOTEBOOK(win->vmbar), index);

    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(win->vmlist));
    g_signal_handlers_block_by_func(G_OBJECT(select), G_CALLBACK(vm_list_selection_changed), NULL);
    delete_vm_for_vmlist(vmName);
    g_signal_handlers_unblock_by_func(G_OBJECT(select), G_CALLBACK(vm_list_selection_changed), NULL);
}
static void deleteVm(void *data)
{
    cJSON *recvInfo = data;
    int status;

    close_progress_dialog(NULL);
    if(NULL == recvInfo)
    {
        dialog_for_message("返回信息异常，解析失败!");
        return ;
    }
    
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        deleteVm_success(current_conn_vm);
        dialog_for_message(_("虚拟机删除成功!"));
    } else {
        
        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }

    cJSON_Delete(recvInfo);
}

static void* start_connect_vm(void *data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        g_idle_add(dialog_for_message, _("资源繁忙,请稍后再试!"));
        return NULL;
    }
    
    cJSON *recvInfo, *connvm;
    char *host, *vmName=data, *vmId;
    int status;

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    vmId = get_cjson_data(vmid_list, vmName);
            
    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/connectVm");
    params = g_string_new("vmId=");
    g_string_append(params, vmId);

    pthread_mutex_unlock(&lock);
    curl_send_post("post_conn_start");
    pthread_mutex_trylock(&lock);
    g_idle_add(close_progress_dialog, NULL);
    
    recvInfo = cjson_recv;
    if(NULL == recvInfo)
    {
        g_free(host);
        dialog_for_message(_("获取连接信息失败\n请稍后重试!"));
        pthread_mutex_unlock(&lock);
        return NULL;
    }
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        connvm = cJSON_GetObjectItem(recvInfo, "data");
        g_idle_add(connect_vm_go, connvm);
        
        sleep(2);
        pthread_mutex_unlock(&lock);
        g_idle_add(get_allVmInfo, NULL);
        
    } else {
 
        if(-6 == status)
        {
            dialog_for_message(_("登录异常\n连接已断开!"));
        } else {
            
            char *error = get_cjson_data(recvInfo, "message");
            dialog_for_message(error);
        }
        g_free(host);
        cJSON_Delete(recvInfo); 
        pthread_mutex_unlock(&lock);
        return NULL;
    }

    g_free(host);
    cJSON_Delete(recvInfo);
    pthread_mutex_unlock(&lock);
    return NULL;
}
static void menu_cb_RunOnce(GtkToolButton *toolbutton, void *data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return ;
    }
    g_source_remove(timer);
    
    char *host, *vmName, *vmId;
    cJSON *recvInfo, *isolist, *lead;
    int index, status;
    char *lead1, *lead2, *lead3, *iso, *c_status;

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    index = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->vmbar));
    vmName = get_vmbar_vmname(index);
    vmId = get_cjson_data(vmid_list, vmName);
    strcpy(current_conn_vm, vmName);

    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/isolist");

    pthread_mutex_unlock(&lock);
    GThread *td = g_thread_new ("get_isolist", curl_send_get, "get_isolist");
    g_thread_join(td);

    pthread_mutex_trylock(&lock);
        
    recvInfo = cjson_recv;
    if(NULL == recvInfo)
    {
        g_free(host);
        dialog_for_message(_("获取ISO镜像列表失败!"));
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        pthread_mutex_unlock(&lock);
        return ;
    }
    c_status = cJSON_PrintUnformatted(cJSON_GetObjectItem(recvInfo, "status"));
    if(NULL != c_status)
    {
        status = atoi(c_status);
        free(c_status);
        if(-6 == status)
            dialog_for_message(_("登录异常\n连接已断开!"));
        else
        {
            char *error = get_cjson_data(recvInfo, "message");
            dialog_for_message(error);
            timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        }
        g_free(host);
        cJSON_Delete(recvInfo); 
        pthread_mutex_unlock(&lock);
        return ;
    }
    isolist = cJSON_GetObjectItem(recvInfo, "domlist");        
    lead = dialog_for_runOnce(isolist);
    if(NULL == lead)
    {
        g_free(host);
        cJSON_Delete(recvInfo);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        pthread_mutex_unlock(&lock);
        return ;
    }
    lead1 = get_cjson_data(lead, "lead1");
    lead2 = get_cjson_data(lead, "lead2");
    lead3 = get_cjson_data(lead, "lead3");
    iso = get_cjson_data(lead, "iso");

    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/runonce");
    params = g_string_new("vmId=");
    g_string_append(params, vmId);
    g_string_append(params, "&bootOrder1=");
    g_string_append(params, lead1);
    g_string_append(params, "&bootOrder2=");
    g_string_append(params, lead2);
    g_string_append(params, "&bootOrder3=");
    g_string_append(params, lead3);
    g_string_append(params, "&bootFile=");
    g_string_append(params, iso);

    pthread_mutex_unlock(&lock);
    g_thread_new("post_runOnce", curl_send_post, "post_runOnce");

    g_free(host);
    cJSON_Delete(lead);
    cJSON_Delete(recvInfo);
    dialog_for_progress(_("运行一次启动中:"));
}
static void menu_cb_ChangeCd(GtkToolButton *toolbutton, void *data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return ;
    }
    g_source_remove(timer);
    
    char *host, *vmName, *vmId, *iso, *c_status;
    int index, status;
    cJSON *recvInfo, *isolist;

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    index = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->vmbar));
    vmName = get_vmbar_vmname(index);
    vmId = get_cjson_data(vmid_list, vmName);
    strcpy(current_conn_vm, vmName);

    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/isolist");

    pthread_mutex_unlock(&lock);
    GThread *td = g_thread_new ("get_isolist", curl_send_get, "get_isolist");
    g_thread_join(td);

    pthread_mutex_trylock(&lock); 
    recvInfo = cjson_recv;
    if(NULL == recvInfo)
    {
        g_free(host);
        dialog_for_message(_("获取ISO镜像列表失败!"));
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        pthread_mutex_unlock(&lock); 
        return ;
    }
    c_status = cJSON_PrintUnformatted(cJSON_GetObjectItem(recvInfo, "status"));
    if(NULL != c_status)
    {
        status = atoi(c_status);
        free(c_status);
        if(-6 == status)
            dialog_for_message(_("登录异常\n连接已断开!"));
        else
        {
            char *error = get_cjson_data(recvInfo, "message");
            dialog_for_message(error);
            timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        }
        g_free(host);
        cJSON_Delete(recvInfo); 
        pthread_mutex_unlock(&lock);
        return ;
    }
     
    isolist = cJSON_GetObjectItem(recvInfo, "domlist");        
    iso = dialog_for_changeCd(isolist);
    if(NULL == iso)
    {
        g_free(host);
        cJSON_Delete(recvInfo);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        pthread_mutex_unlock(&lock);
        return ;
    }
    
    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/changecd");
    params = g_string_new("vmId=");
    g_string_append(params, vmId);
    g_string_append(params, "&newCD=");
    g_string_append(params, iso);

    pthread_mutex_unlock(&lock);
    g_thread_new("post_changecd", curl_send_post, "post_changecd");

    g_free(host);
    g_free(iso);
    cJSON_Delete(recvInfo);
    dialog_for_progress(_("正在切换CD:"));
}
static void changeCd(void *data)
{
    cJSON *recvInfo = data;
    int status;  

    close_progress_dialog(NULL);
    if(NULL != recvInfo)
    {
        status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
        if(0 == status)
            dialog_for_message(_("切换CD成功!"));          
        else
        {
            char *error = get_cjson_data(recvInfo, "message");
            dialog_for_message(error);    
        }
    } else {
        dialog_for_message(_("获取ISO镜像列表失败!"));
        return ;
    }

    cJSON_Delete(recvInfo);
}

static void menu_cb_StartVm(GtkToolButton *toolbutton, void *data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return ;
    }
    g_source_remove(timer);
    
    char *host, *vmName, *vmId;
    int index;

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    index = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->vmbar));
    vmName = get_vmbar_vmname(index);
    vmId = get_cjson_data(vmid_list, vmName);
    strcpy(current_conn_vm, vmName);

    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/run");
    params = g_string_new("vmId=");
    g_string_append(params, vmId);

    pthread_mutex_unlock(&lock);
    g_thread_new("post_start", curl_send_post, "post_start");

    g_free(host);
    dialog_for_progress(_("虚拟机启动中:"));
}
static void startVm(void *data)
{
    cJSON *recvInfo = data;
    int status;  

    if(NULL == recvInfo)
    {   
        close_progress_dialog(NULL);
        dialog_for_message("返回信息异常，解析失败!");
        return ;
    }

    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        pthread_mutex_unlock(&lock);
        g_thread_new("startVm", start_connect_vm, current_conn_vm);
        
    } else {

        close_progress_dialog(NULL);
        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }

    cJSON_Delete(recvInfo);
}

static void menu_cb_PauseVm(GtkToolButton *toolbutton, void *data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return ;
    }
    g_source_remove(timer);
    
    if(!dialog_for_confirm(_("是否暂停虚拟机?"), NULL))  
    {
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return;
    }
    
    char *host, *vmName, *vmId;
    int index;

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    index = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->vmbar));
    vmName = get_vmbar_vmname(index);
    vmId = get_cjson_data(vmid_list, vmName);
    strcpy(current_conn_vm, vmName);
    
    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/connectVm");
    params = g_string_new("vmId=");
    g_string_append(params, vmId);

    pthread_mutex_unlock(&lock);
    g_thread_new("post_take_shot", curl_send_post, "post_take_shot");
    
    g_free(host);
    dialog_for_progress(_("虚拟机暂停中:"));
}
static void pause_take_shot(void *data)
{
    char *host, *hostIp, *tls_port, *passwd, *vmId;
    GString *cmd;
    int status, ret=0;
    cJSON *recvInfo = data;

    if(NULL == recvInfo)
    {    
        close_progress_dialog(NULL);
        dialog_for_message("返回信息异常，解析失败!"); 
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return ;
    }
    
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        cJSON *vm = cJSON_GetObjectItem(recvInfo, "data");
        host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
        hostIp = get_cjson_data(vm, "ip");  
        tls_port = get_cjson_data(vm, "tls-port");
        passwd = get_cjson_data(vm, "passwd");

        cmd = g_string_new("linxVirtScreenShot -h ");
        g_string_append(cmd, hostIp);
        g_string_append(cmd, " -s ");
        g_string_append(cmd, tls_port);
        g_string_append(cmd, " -w ");
        g_string_append(cmd, passwd);
        g_string_append(cmd, " -o ");
        g_string_append(cmd, "/etc/linx/ppms/ppm/");
        g_string_append(cmd, current_conn_vm);
        g_string_append(cmd, ".ppm");

        conn_info *ci = get_conn_from_list_by_conn(win->conn);
        if(NULL != ci)
        {
            g_signal_handlers_block_by_func(ci->channel, G_CALLBACK(main_channel_event), win->conn);
            g_signal_handlers_block_by_func(ci->channel, G_CALLBACK(main_agent_update), win->conn);
        }
    
        ret = system(cmd->str);
        g_string_free(cmd, TRUE); 
        if ( 0 != ret )
        {   
            close_progress_dialog(NULL);
            dialog_for_message("暂停失败,保存虚拟机状态出错!");
            g_free(host);
            cJSON_Delete(recvInfo);
            if(NULL != ci)
            {
                g_signal_handlers_unblock_by_func(ci->channel, G_CALLBACK(main_channel_event), win->conn);
                g_signal_handlers_unblock_by_func(ci->channel, G_CALLBACK(main_agent_update), win->conn);
            }
            timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
            return ;
        }

        vmId = get_cjson_data(vmid_list, current_conn_vm);
        url = g_string_new("https://");
        g_string_append(url, host);
        g_string_append(url, ":8443/vm/pause");
        params = g_string_new("vmId=");
        g_string_append(params, vmId);

        pthread_mutex_unlock(&lock);
        g_thread_new("post_pause", curl_send_post, "post_pause");

        g_free(host);
        
    } else {

        close_progress_dialog(NULL);
        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }
    
    cJSON_Delete(recvInfo);
}
static void pauseVm(void *data)
{
    cJSON *recvInfo = data;
    int status;

    close_progress_dialog(NULL);
    conn_info *ci = get_conn_from_list_by_conn(win->conn);
    if(NULL == recvInfo)
    {
        dialog_for_message("返回信息异常，解析失败!");
        if(NULL != ci)
        {
            ci->conn->wins[ci->id * CHANNELID_MAX + ci->monitor_id] = NULL;
            connection_disconnect(ci->conn);
            conn_list = g_slist_remove(conn_list, ci);
            g_free(ci->vmName);
            g_free(ci);
        }
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        connect_vm(current_conn_vm);
        return ;
    }

    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        IS_VM_PAUSE = TRUE;
        while_conn_shutdown(win->conn);     
        pthread_mutex_unlock(&lock);
        update_vmlist_status(current_conn_vm, 3);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        
    } else {
        
        char *error = get_cjson_data(recvInfo, "message");
        if(NULL != ci)
        {
            ci->conn->wins[ci->id * CHANNELID_MAX + ci->monitor_id] = NULL; 
            connection_disconnect(ci->conn);
            conn_list = g_slist_remove(conn_list, ci);
            g_free(ci->vmName);
            g_free(ci);
        }

        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        connect_vm(current_conn_vm);
        dialog_for_message(error);
    }

    cJSON_Delete(recvInfo);
}

static void menu_cb_StopVm(GtkToolButton *toolbutton, void *data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return ;
    }
    g_source_remove(timer);
    
    if(!dialog_for_confirm(_("是否关闭虚拟机?"), NULL))  
    {
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return;
    }
    
    char *host, *vmName, *vmId;
    int index;
    conn_info *ci;

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    index = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->vmbar));
    vmName = get_vmbar_vmname(index);
    vmId = get_cjson_data(vmid_list, vmName);
    
    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/destroy");
    params = g_string_new("vmId=");
    g_string_append(params, vmId);

    ci = get_conn_from_list_by_conn(win->conn);
    if(NULL != ci)
    {
        g_signal_handlers_block_by_func(ci->channel, G_CALLBACK(main_channel_event), win->conn);
        g_signal_handlers_block_by_func(ci->channel, G_CALLBACK(main_agent_update), win->conn);
    }

    pthread_mutex_unlock(&lock);
    g_thread_new("post_stop", curl_send_post, "post_stop");
    
    g_free(host);
    dialog_for_progress(_("虚拟机关闭中:"));
}
static void stopVm(void *data)
{
    cJSON *recvInfo = data;
    int status; 
    conn_info *ci;

    close_progress_dialog(NULL);
    ci = get_conn_from_list_by_conn(win->conn);
    if(NULL == recvInfo)
    {       
        if(NULL != ci)
        {
            g_signal_handlers_unblock_by_func(ci->channel, G_CALLBACK(main_channel_event), win->conn);
            g_signal_handlers_unblock_by_func(ci->channel, G_CALLBACK(main_agent_update), win->conn);
        }
        dialog_for_message("返回信息异常，解析失败!");
        return ;
    }
    
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        IS_VM_PAUSE = FALSE;
        while_conn_shutdown(win->conn);
        pthread_mutex_unlock(&lock);
        get_allVmInfo(NULL);
        
    } else {

        if(NULL != ci)
        {
            g_signal_handlers_unblock_by_func(ci->channel, G_CALLBACK(main_channel_event), win->conn);
            g_signal_handlers_unblock_by_func(ci->channel, G_CALLBACK(main_agent_update), win->conn);
        }
        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }

    cJSON_Delete(recvInfo);
}

static void menu_cb_RestartVm(GtkToolButton *toolbutton, void *data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return ;
    }
    g_source_remove(timer);
    
    if(!dialog_for_confirm(_("是否重启虚拟机?"), NULL))  
    {
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return;
    }
    
    char *host, *vmName, *vmId;
    conn_info *ci;
    int index;

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    index = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->vmbar));
    vmName = get_vmbar_vmname(index);
    vmId = get_cjson_data(vmid_list, vmName);
    strcpy(current_conn_vm, vmName);

    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/reboot");
    params = g_string_new("vmId=");
    g_string_append(params, vmId);

    ci = get_conn_from_list_by_conn(win->conn);
    
    if(NULL != ci)
    {
        g_signal_handlers_block_by_func(ci->channel, G_CALLBACK(main_channel_event), win->conn);
        g_signal_handlers_block_by_func(ci->channel, G_CALLBACK(main_agent_update), win->conn);
    }
    
    pthread_mutex_unlock(&lock);
    g_thread_new("post_restart", curl_send_post, "post_restart");

    g_free(host);
    dialog_for_progress(_("虚拟机重启中:"));
}
static void restartVm(void *data)
{
    cJSON *recvInfo = data;
    int status;
    conn_info *ci;

    ci = get_conn_from_list_by_conn(win->conn);
    if(NULL == recvInfo)
    {        
        close_progress_dialog(NULL);
        dialog_for_message("返回信息异常，解析失败!");
        if(NULL != ci)
        {
            g_signal_handlers_unblock_by_func(ci->channel, G_CALLBACK(main_channel_event), win->conn);
            g_signal_handlers_unblock_by_func(ci->channel, G_CALLBACK(main_agent_update), win->conn);
        }
        return ;
    }
    
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        if(ci != NULL)
        {
            ci->conn->wins[ci->id * CHANNELID_MAX + ci->monitor_id] = NULL;
            connection_disconnect(ci->conn);
            conn_list = g_slist_remove(conn_list, ci);
            g_free(ci->vmName);
            g_free(ci);
        }
        pthread_mutex_unlock(&lock);
        g_thread_new("restartVm", start_connect_vm, current_conn_vm);
        
    } else {

        if(NULL != ci)
        {
            g_signal_handlers_unblock_by_func(ci->channel, G_CALLBACK(main_channel_event), win->conn);
            g_signal_handlers_unblock_by_func(ci->channel, G_CALLBACK(main_agent_update), win->conn);
        }
        close_progress_dialog(NULL);
        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }

    cJSON_Delete(recvInfo);
}

static cJSON* dialog_for_newsnap(void);
static int dialog_for_manageSnap(void *snaps);
static snapshot_info* get_snapshot_info(cJSON *snapShots,
                                        char *name, char *srcUUID);
static cJSON *snapShots = NULL;
static GSList *snaps_list = NULL;

static void menu_cb_SaveSnapShot(GtkToolButton *toolbutton, void *data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return ;
    }
    g_source_remove(timer);
    
    char *snapName, *description;
    char *host, *vmName, *vmId;
    int index, status;
    GThread *td;
    cJSON *snapInfo, *snaps, *tmp;

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    index = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->vmbar));
    vmName = get_vmbar_vmname(index);
    vmId = get_cjson_data(vmid_list, vmName);
    
    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/snapshottree");
    params = g_string_new("vmId=");
    g_string_append(params, vmId);
    
    pthread_mutex_unlock(&lock);
    td = g_thread_new("post_check_snap", curl_send_post, "post_check_snap");
    g_thread_join(td);
    if(NULL == cjson_recv)
    {
        dialog_for_message(_("获取虚拟机已有快照信息失败\n请稍后重试!"));
        g_free(host);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        pthread_mutex_unlock(&lock);
        return ;
    }
    status = cJSON_GetObjectItem(cjson_recv, "status")->valueint;
    if(0 != status)
    {
        if(-6 == status)
            dialog_for_message(_("登录异常\n连接已断开!"));
        else
        {
            dialog_for_message(_("获取虚拟机已有快照信息失败\n请稍后重试!"));
            timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        }
        g_free(host);
        cJSON_Delete(cjson_recv);
        pthread_mutex_unlock(&lock);
        return ;
    }
    
    if(0 == status)
    {
        snaps = cJSON_GetObjectItem(cjson_recv, "snapVolumes");
        if(snaps != NULL)
        {
            for(int i=0; i<cJSON_GetArraySize(snaps); i++)
            {
                tmp = cJSON_GetArrayItem(snaps, i);
                if(0 == cJSON_GetObjectItem(tmp, "active")->valueint)
                {
                    if(0 != cJSON_GetObjectItem(tmp, "top")->valueint)
                    {
                        dialog_for_message(_("当前系统基于快照有后续快照\n不能再创建快照!"));
                        g_free(host);
                        cJSON_Delete(cjson_recv);
                        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
                        pthread_mutex_unlock(&lock);
                        return ;
                    }
                }
            }
        }
    }
    cJSON_Delete(cjson_recv);
    
    pthread_mutex_trylock(&lock);
    snapInfo = dialog_for_newsnap();
    if(NULL == snapInfo)
    {
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        g_free(host);
        pthread_mutex_unlock(&lock);
        return ;
    }
    snapName = get_cjson_data(snapInfo, "name");
    description = get_cjson_data(snapInfo, "notes");

    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/snapshot");
    
    params = g_string_new("name=");
    g_string_append(params, snapName);
    g_string_append(params, "&description=");
    g_string_append(params, description);
    g_string_append(params, "&vmId=");
    g_string_append(params, vmId);

    pthread_mutex_unlock(&lock);
    g_thread_new("post_save_snap", curl_send_post, "post_save_snap");

    g_free(host);
    cJSON_Delete(snapInfo);
    dialog_for_progress(_("正在创建快照:"));
}
static void saveSnapShot(void *data)
{   
    cJSON *recvInfo = data;
    int status;

    close_progress_dialog(NULL);
    if(NULL == recvInfo)
    {
        dialog_for_message(_("返回信息异常，解析失败!"));
        return ;
    }
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        dialog_for_message(_("快照创建成功!"));
    } else {
        
        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }

    cJSON_Delete(recvInfo);
}
static void menu_cb_ManageSnapShot(GtkToolButton *toolbutton, void *data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return ;
    }
    g_source_remove(timer);

    char *host, *vmName, *vmId;
    int index;

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    index = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->vmbar));
    vmName = get_vmbar_vmname(index);
    vmId = get_cjson_data(vmid_list, vmName);

    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/snapshottree?");
    g_string_append(url, "vmId=");
    g_string_append(url, vmId);

    pthread_mutex_unlock(&lock);
    g_thread_new("get_snap_tree", curl_send_get, "get_snap_tree");

    g_free(host);
}
static void manageSnapShot(void *data)
{   
    cJSON *recvInfo = data;
    int status;
    snapshot_info *snap;

    if(NULL == recvInfo)
    {
        dialog_for_message(_("返回信息异常，解析失败!"));
        return ;
    }
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        snapShots = cJSON_GetObjectItem(recvInfo, "snapVolumes");
        dialog_for_manageSnap(snapShots);
        pthread_mutex_unlock(&lock);
        
    } else {
        
        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }

    if(!IS_DEL)
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
    IS_DEL = FALSE;

    while(snaps_list != NULL)
    {
        snap = snaps_list->data;
        snaps_list = g_slist_remove(snaps_list, snap);
        free(snap);
    }
    g_slist_free(snaps_list);
    cJSON_Delete(recvInfo);
}
static void on_delSnapShot_bun_clicked(GtkButton *button, gpointer data)
{
    char *host, *vmName, *vmId;
    int index;
    snapshot_info *snap;
    GtkWidget *snap_entry = data;
    GString *cmd;

    const char *snap_name = gtk_entry_get_text(GTK_ENTRY(snap_entry));
    snap = get_snapshot_info(snapShots, (char*)snap_name, NULL);

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    index = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->vmbar));
    vmName = get_vmbar_vmname(index);
    vmId = get_cjson_data(vmid_list, vmName);
    strcpy(current_conn_vm, vmName);

    cmd = g_string_new("是否删除该虚拟机: ");
    g_string_append(cmd, vmName);
    g_string_append(cmd, " 的快照: ");
    g_string_append(cmd, (char*)snap_name);
    g_string_append(cmd, " ?");
    if( !dialog_for_confirm(cmd->str, NULL))
    {
        free(snap);
        g_free(host);
        g_string_free(cmd, TRUE);
        return ;
    }
    
    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/delsnapshot");
    params = g_string_new("vmId=");
    g_string_append(params, vmId);
    g_string_append(params, "&volUUID=");
    g_string_append(params, snap->volUUID);

    IS_DEL = TRUE;
    pthread_mutex_unlock(&lock);
    g_thread_new("post_del_snap", curl_send_post, "post_del_snap");

    free(snap);
    g_free(host);
    g_string_free(cmd, TRUE);
    dialog_for_progress(_("正在删除快照:"));
}
static void delSnapShot(void* data)
{
    cJSON *recvInfo = data;
    int status;

    close_progress_dialog(NULL);
    if(NULL == recvInfo)
    {
        dialog_for_message(_("返回信息异常，解析失败!"));
        return ;    
    }
    
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        gtk_widget_destroy(snapshots_dialog);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        pthread_mutex_unlock(&lock);
        menu_cb_ManageSnapShot(NULL, NULL); 
        pthread_mutex_unlock(&lock);
        
    } else {

        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }

    cJSON_Delete(recvInfo);
}

static void on_reSnapShot_bun_clicked(GtkButton *button, gpointer data)
{
    char *host, *vmName, *vmId;
    int index;
    snapshot_info *snap;
    GtkWidget *snap_entry = data;
    GString *cmd;

    const char *snap_name = gtk_entry_get_text(GTK_ENTRY(snap_entry));
    snap = get_snapshot_info(snapShots, (char*)snap_name, NULL);

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    index = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->vmbar));
    vmName = get_vmbar_vmname(index);
    vmId = get_cjson_data(vmid_list, vmName);
    strcpy(current_conn_vm, vmName);

    cmd = g_string_new("是否恢复到虚拟机: ");
    g_string_append(cmd, vmName);
    g_string_append(cmd, " 的快照: ");
    g_string_append(cmd, (char*)snap_name);
    g_string_append(cmd, " ?");
    if( !dialog_for_confirm(cmd->str, NULL) )
    {
        free(snap);
        g_free(host);
        g_string_free(cmd, TRUE);
        return ;
    }
    g_string_free(cmd, TRUE);

    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/vm/restoresnapshot");
    params = g_string_new("vmId=");
    g_string_append(params, vmId);
    g_string_append(params, "&restoreVolUUID=");
    g_string_append(params, snap->volUUID);

    conn_info *ci = get_conn_from_list_by_conn(win->conn);
    if(NULL != ci)
    {
        g_signal_handlers_block_by_func(ci->channel, G_CALLBACK(main_channel_event), win->conn);
        g_signal_handlers_block_by_func(ci->channel, G_CALLBACK(main_agent_update), win->conn);
    }
    
    pthread_mutex_unlock(&lock);
    g_thread_new("post_re_snap", curl_send_post, "post_re_snap");

    free(snap);
    g_free(host);
    dialog_for_progress(_("正在恢复虚拟机快照:"));
}
static void reSnapShot(void *data)
{
    cJSON *recvInfo = data;
    int status;
    conn_info *ci;

    ci = get_conn_from_list_by_conn(win->conn);
    if(NULL == recvInfo)
    {
        if(NULL != ci)
        {
            g_signal_handlers_unblock_by_func(ci->channel, G_CALLBACK(main_channel_event), win->conn);
            g_signal_handlers_unblock_by_func(ci->channel, G_CALLBACK(main_agent_update), win->conn);
        }
        close_progress_dialog(NULL);
        dialog_for_message(_("返回信息异常，解析失败!"));
        return ;
    }
    
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        gtk_widget_destroy(snapshots_dialog);
        if(NULL != ci)
        {
            ci->conn->wins[ci->id * CHANNELID_MAX + ci->monitor_id] = NULL;
            connection_disconnect(ci->conn);
            conn_list = g_slist_remove(conn_list, ci);
            g_free(ci->vmName);
            g_free(ci);
        }
        pthread_mutex_unlock(&lock);
        g_thread_new("re_snap", start_connect_vm, current_conn_vm);
        
    } else {

        if(NULL != ci)
        {
            g_signal_handlers_unblock_by_func(ci->channel, G_CALLBACK(main_channel_event), win->conn);
            g_signal_handlers_unblock_by_func(ci->channel, G_CALLBACK(main_agent_update), win->conn);
        }
        close_progress_dialog(NULL);
        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }

    cJSON_Delete(recvInfo);
}

static cJSON* dialog_for_newsnap(void)
{
    GtkWidget *dialog, *area;
    GtkWidget *snapName, *description;
    GtkWidget *snapName1, *description1;
    GtkWidget *hbox, *hbox1;
    GtkTextBuffer *buffer;
    GtkTextIter start, end;
    const char *name;
    char *notes;
    GtkWidget *scrlled_window;
    cJSON *result = NULL;
        
    hbox = gtk_hbox_new(FALSE, 5); 
    hbox1 = gtk_hbox_new(FALSE, 5);
    snapName = gtk_label_new(_("快照名称:"));
    description = gtk_label_new(_("快照描述:"));
    snapName1 = gtk_entry_new();
    description1 = gtk_text_view_new();

    gtk_entry_set_max_length(GTK_ENTRY(snapName1), 32);

    scrlled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(scrlled_window), 5);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrlled_window), 
                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrlled_window), description1);
    gtk_box_pack_start(GTK_BOX(hbox), snapName, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), snapName1, TRUE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(hbox1), description, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox1), scrlled_window, TRUE, TRUE, 8);
    
    dialog = gtk_dialog_new_with_buttons(_("新建虚拟机快照"), GTK_WINDOW(win->toplevel),
                                    GTK_DIALOG_MODAL,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_OK, GTK_RESPONSE_OK,
                                    NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
    gtk_widget_set_size_request(dialog, 380, 260);
        
    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(area), hbox, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox1, TRUE, TRUE, 5);
    gtk_widget_show_all(dialog); 

    int flag;
    while( TRUE )
    {
        flag = 1;
        switch(gtk_dialog_run(GTK_DIALOG(dialog)))
        {
            case GTK_RESPONSE_OK: 
                name = gtk_entry_get_text(GTK_ENTRY(snapName1));
                buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(description1));
                gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(buffer), &start, &end);
                notes = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(buffer), &start, &end, FALSE);

                if(0 == g_strcmp0(name, ""))
                {
                    dialog_for_message(_("请输入快照名!"));
                    break ;
                }
                result = cJSON_CreateObject();
                cJSON_AddStringToObject(result, "name", name);
                cJSON_AddStringToObject(result, "notes", notes);
                flag = 0;
                free(notes);
                break ;
            
            case GTK_RESPONSE_CANCEL:
                flag = 0;
                break ;
            default:
                flag = 0;
                break ;
        }
        if(0 == flag)
            break ;
    }
    gtk_widget_destroy(dialog);
    return result;
}
static GtkWidget* new_snapshot_bun(char *data, int active)
{
    GtkWidget *bun;
    GtkWidget *vbox;
    GdkPixbuf *pixbuf;
    GtkWidget *image;
    GtkWidget *label;
    
    bun = gtk_button_new();
    vbox = gtk_vbox_new(FALSE, 3);
    pixbuf = gdk_pixbuf_new_from_file("/etc/linx/ppms/png/home.png", NULL);
    image = gtk_image_new_from_pixbuf(pixbuf);
    label = gtk_label_new(data);
    gtk_label_set_width_chars(GTK_LABEL(label), 3);
    
    gtk_container_add(GTK_CONTAINER(bun), vbox);
    gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 1);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 1);

    if(1 == active)
    {
        GtkWidget *label1 = gtk_label_new("(系统位置)");
        gtk_box_pack_start(GTK_BOX(vbox), label1, FALSE, FALSE, 1);
    }
    
    g_object_unref(pixbuf);
    return bun;
}
static snapshot_info* get_snapshot_info(cJSON *snapShots, char *name, char *srcUUID)
{
    char *cmp, *cmp1, *snapName, *srcVolUUID;
    int i, len;
    len = cJSON_GetArraySize(snapShots);
    cJSON *tmp;
    snapshot_info *snap;
    if(NULL == (snap = malloc(sizeof(struct snapshot_info))))
    {
        g_print("get_snapshot_info(): malloc failed!\n");
        dialog_for_message("malloc failed!");
        return NULL; 
    }

    for(i=0; i<len; i++)
    {
        tmp = cJSON_GetArrayItem(snapShots, i);
        srcVolUUID = get_cjson_data(tmp, "srcVolUUID");
        snapName = get_cjson_data(tmp, "name");
        if(NULL == name)
        {
            cmp = srcUUID;
            cmp1 = srcVolUUID;
        }
        else
        {
            cmp = name;
            cmp1 = snapName;
        }
        if(0 == g_strcmp0(cmp, cmp1))
        {
            snap->name = snapName;
            snap->description = get_cjson_data(tmp, "description");
            snap->imgGUID = get_cjson_data(tmp, "imgGUID");
            snap->volUUID = get_cjson_data(tmp, "volUUID");
            snap->srcVolUUID = srcVolUUID;
            snap->active = cJSON_GetObjectItem(tmp, "active")->valueint;
            snap->top = cJSON_GetObjectItem(tmp, "top")->valueint;
        }
    }
    return snap;
}
static void on_snapshot_bun_clicked(GtkToggleButton *togglebutton, gpointer data)
{
    snapshot_info *snap = data;
    char *name, *description;
    GtkWidget *del_bun, *re_bun, *name_entry, *description_entry;
    GtkTextBuffer *buffer;

    del_bun = snap->del_bun;
    re_bun = snap->re_bun;
    name = snap->name;
    description = snap->description;
    name_entry = snap->name_entry;
    description_entry = snap->description_entry;

    gtk_entry_set_text(GTK_ENTRY(name_entry), name);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(description_entry));

    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), description, -1);
    if( !gtk_widget_get_sensitive(del_bun) )
        gtk_widget_set_sensitive(del_bun, TRUE);
    if( !gtk_widget_get_sensitive(re_bun) )
        gtk_widget_set_sensitive(re_bun, TRUE);
}
static GtkWidget* init_snapshot_tree(cJSON *snaps, 
                                   GtkWidget *del_bun, GtkWidget *re_bun,
                                   GtkWidget *name_entry, GtkWidget *description_entry)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *scrlled_window;
    GdkPixbuf *pixbuf;
    GtkWidget *right, *right_end, *snap_bun, *end;
    snapshot_info *snap;
    GSList *list = NULL;
    
    vbox = gtk_vbox_new(FALSE, 3);
    hbox = gtk_hbox_new(FALSE, 3);
    pixbuf = gdk_pixbuf_new_from_file("/etc/linx/ppms/png/shot.png", NULL);

    if(0 == cJSON_GetArraySize(snapShots))
    {
        GtkWidget *label = gtk_label_new("虚拟机未创建快照！");
        gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 1);
        
    } else {

        snap = get_snapshot_info(snapShots, NULL, "00000000-0000-0000-0000-000000000000");
        if(NULL == snap)
            return NULL;
        while(TRUE)
        {
            list = g_slist_append(list, snap);
            snap->del_bun = del_bun;
            snap->re_bun = re_bun;
            snap->name_entry = name_entry;
            snap->description_entry = description_entry;
            right = gtk_image_new_from_pixbuf(pixbuf);
            snap_bun = new_snapshot_bun(snap->name, snap->active);
            g_signal_connect(snap_bun, "clicked", 
                             G_CALLBACK(on_snapshot_bun_clicked), snap);
            gtk_box_pack_start(GTK_BOX(hbox), right, FALSE, FALSE, 3);
            gtk_box_pack_start(GTK_BOX(hbox), snap_bun, FALSE, FALSE, 3);
            if(1 == snap->top) 
                break ;
            snap = get_snapshot_info(snapShots, NULL, snap->volUUID);
        }          
        right_end = gtk_image_new_from_pixbuf(pixbuf);
        end = gtk_image_new_from_stock(GTK_STOCK_STOP, GTK_ICON_SIZE_DND);
        gtk_box_pack_start(GTK_BOX(hbox), right_end, FALSE, FALSE, 3);
        gtk_box_pack_start(GTK_BOX(hbox), end, FALSE, FALSE, 3);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 1);
    }
    scrlled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(scrlled_window), 15);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrlled_window), 
                                           GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrlled_window), vbox);
    gtk_widget_set_size_request(scrlled_window, 550, 300);
    gtk_widget_show_all(scrlled_window);

    snaps_list = list;
    g_object_unref(pixbuf);
    return scrlled_window;
}
static int dialog_for_manageSnap(void *snaps)
{
    GtkWidget *area;
    GtkWidget *hbox, *scrlled_window;
    GtkWidget *del_bun, *re_bun;

    GtkWidget *snapName, *description;
    GtkWidget *snapName1, *description1;
    GtkWidget *hbox1, *hbox2;
    GtkWidget *scrlled_window1;

    hbox = gtk_hbox_new(FALSE, 0);
    del_bun = new_image_bun(GTK_STOCK_UNDELETE, _("删除"));
    re_bun = new_image_bun(GTK_STOCK_REFRESH, _("恢复"));
    gtk_box_pack_start(GTK_BOX(hbox), del_bun, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), re_bun, FALSE, FALSE, 5);

    hbox1 = gtk_hbox_new(FALSE, 0);
    hbox2 = gtk_hbox_new(FALSE, 0);
    snapName = gtk_label_new(_("快照名称:"));
    description = gtk_label_new(_("快照描述:"));
    snapName1 = gtk_entry_new();
    description1 = gtk_text_view_new();
    gtk_widget_set_sensitive(snapName1, FALSE);
    gtk_widget_set_sensitive(description1, FALSE);

    scrlled_window = init_snapshot_tree(snaps, del_bun, re_bun, snapName1, description1);
    scrlled_window1 = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(scrlled_window1), 5);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrlled_window1), 
                                   GTK_POLICY_AUTOMATIC, 
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrlled_window1), description1);
    gtk_widget_set_size_request(scrlled_window1, 350, 100);
    gtk_widget_show_all(scrlled_window1);
    
    gtk_box_pack_start(GTK_BOX(hbox1), snapName, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox1), snapName1, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox2), description, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox2), scrlled_window1, TRUE, TRUE, 0);
    
    snapshots_dialog = gtk_dialog_new_with_buttons(_("快照管理"), GTK_WINDOW(win->toplevel),
                                    GTK_DIALOG_MODAL,
                                    GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                    NULL);
    gtk_widget_set_size_request(snapshots_dialog, 600, 550);
        
    area = gtk_dialog_get_content_area(GTK_DIALOG(snapshots_dialog));
    gtk_box_pack_start(GTK_BOX(area), hbox, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(area), scrlled_window, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox1, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(area), hbox2, TRUE, TRUE, 5);

    g_signal_connect(del_bun, "clicked", 
                    G_CALLBACK(on_delSnapShot_bun_clicked),
                    snapName1);
    g_signal_connect(re_bun, "clicked", 
                    G_CALLBACK(on_reSnapShot_bun_clicked),
                    snapName1);

    gtk_widget_set_sensitive(del_bun, FALSE);
    gtk_widget_set_sensitive(re_bun, FALSE);    
    
    gtk_widget_show_all(snapshots_dialog); 
    gtk_dialog_run(GTK_DIALOG(snapshots_dialog));
    
    if(GTK_IS_WIDGET(snapshots_dialog))
        gtk_widget_destroy(snapshots_dialog);

    return 0;
}

static void on_delImage_bun_clicked(GtkButton *bun, gpointer data)
{
    GtkTreeSelection *select=data;
    GtkTreeIter iter;
    GtkTreeModel* model;
    char *host, *name, *id;
    int type=0;

    if(!dialog_for_confirm(_("确定删除磁盘?"), &type))
        return ;
    
    if(gtk_tree_selection_get_selected(select, &model, &iter))
        gtk_tree_model_get(model, &iter, 0, &name, 2, &id, -1);

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));

    /* url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/storage/getdisktype?");
    g_string_append(url, "imageUUID=");
    g_string_append(url, id);

    pthread_mutex_unlock(&lock);
    GThread *td = g_thread_new("get_img_type", curl_send_get, "get_img_type");
    g_thread_join(td);
    pthread_mutex_trylock(&lock);
    recvInfo = cjson_recv;
    if(NULL == recvInfo)
    {
        g_free(host);
        g_free(name);
        g_free(id);
        dialog_for_message(_("获取磁盘类型失败\n删除磁盘失败!"));
        return ;
    }
    type = atoi(cJSON_GetObjectItem(recvInfo, "diskType")->valuestring);  
    if(-1 == type)
    {
        g_free(host);
        g_free(name);
        g_free(id);
        cJSON_Delete(recvInfo);
        dialog_for_message(_("获取磁盘类型失败\n删除磁盘失败!"));
        return ;
    } */

    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/storage/delvmdisk");

    params = g_string_new("diskAlias=");
    g_string_append(params, name);
    g_string_append(params, "&wipe=");
    if(1 == type)
        g_string_append(params, "True");
    else
        g_string_append(params, "False");

    IS_DEL = TRUE;
    pthread_mutex_unlock(&lock);
    g_thread_new("post_del_image", curl_send_post, "post_del_image");

    g_free(host);
    g_free(name);
    g_free(id);
    dialog_for_progress(_("正在删除磁盘:"));
}
static void manage_image_list_select(GtkTreeSelection *treeselection, gpointer data)
{
    GtkWidget *bun=(GtkWidget *)data;
    
    gtk_widget_set_sensitive(bun, TRUE);
    return ;
}
static void dialog_for_manageImage(void *data)
{
    GtkWidget *area, *scrlled_window, *hbox, *del_bun;
    cJSON *images=data, *tmp;
    char *name, *notes, *id, *attach, *size, c_type[30];
    int type;
    
    GtkWidget *tree;
    GtkTreeViewColumn *column1, *column2, *column3, *column4, *column5, *column6;
    GtkCellRenderer *renderer_txt;
    GtkTreeStore *store;
    GtkTreeIter iter;
    GtkTreeSelection *select;

    store = gtk_tree_store_new(6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    for(int i=0; i<cJSON_GetArraySize(images); i++)
    {
        tmp = cJSON_GetArrayItem(images, i);
        
        name = cJSON_GetObjectItem(tmp, "diskAlias")->valuestring;
        notes = cJSON_GetObjectItem(tmp, "description")->valuestring;
        id = cJSON_GetObjectItem(tmp, "imageUUID")->valuestring;
        attach = cJSON_GetObjectItem(tmp, "attached")->valuestring;
        size = cJSON_GetObjectItem(tmp, "size")->valuestring;
        type = cJSON_GetObjectItem(tmp, "attachType")->valueint;
        if(-1 == type)
            strcpy(c_type, "未附加");
        if(0 == type)
            strcpy(c_type, "附加到虚拟机");
        if(1 == type)
            strcpy(c_type, "附加到模板");
            
        gtk_tree_store_append(store, &iter, NULL);
        gtk_tree_store_set(store, &iter, 0, name, 1, notes, 2, id, 3, attach,
                                                 4, size, 5, c_type, -1);
    }
    
    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    renderer_txt = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer_txt), "foreground", "black", NULL);
    column1 = gtk_tree_view_column_new_with_attributes("名称",
                                                       renderer_txt,
                                                       "text",
                                                       0,
                                                       NULL);
    column2 = gtk_tree_view_column_new_with_attributes("注释",
                                                       renderer_txt,
                                                       "text",
                                                       1,
                                                       NULL);
    column3 = gtk_tree_view_column_new_with_attributes("磁盘ID",
                                                       renderer_txt,
                                                       "text",
                                                       2,
                                                       NULL);
    column4 = gtk_tree_view_column_new_with_attributes("附加到的实例",
                                                       renderer_txt,
                                                       "text",
                                                       3,
                                                       NULL);
    column5 = gtk_tree_view_column_new_with_attributes("磁盘大小",
                                                       renderer_txt,
                                                       "text",
                                                       4,
                                                       NULL);
    column6 = gtk_tree_view_column_new_with_attributes("类型",
                                                       renderer_txt,
                                                       "text",
                                                       5,
                                                       NULL);
    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);

    gtk_tree_view_column_set_resizable(column1, TRUE);
    gtk_tree_view_column_set_resizable(column2, TRUE);
    gtk_tree_view_column_set_resizable(column3, TRUE);
    gtk_tree_view_column_set_resizable(column4, TRUE);
    gtk_tree_view_column_set_resizable(column5, TRUE);
    gtk_tree_view_column_set_resizable(column6, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column2);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column3);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column4);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column5);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column6);

    images_dialog = gtk_dialog_new_with_buttons(_("磁盘管理"), GTK_WINDOW(win->toplevel),
                                    GTK_DIALOG_MODAL,
                                    GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                    NULL);
    scrlled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(scrlled_window), 5);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrlled_window), 
                                        GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrlled_window), tree);
    
    gtk_widget_set_size_request(images_dialog, 850, 500);  
    gtk_widget_set_size_request(scrlled_window, 820, 355);
    area = gtk_dialog_get_content_area(GTK_DIALOG(images_dialog));
    gtk_box_pack_start(GTK_BOX(area), scrlled_window, TRUE, TRUE, 10); 
    hbox = gtk_hbox_new(FALSE, 5);
    del_bun = gtk_button_new_with_label(_("删除"));
    gtk_widget_set_sensitive(del_bun, FALSE);
    gtk_box_pack_start(GTK_BOX(area), hbox, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(hbox), del_bun, FALSE, FALSE, 10);

    g_signal_connect(G_OBJECT(del_bun),
                     "clicked",
                     G_CALLBACK(on_delImage_bun_clicked),
                     select);
    g_signal_connect(G_OBJECT(select),
                     "changed",
                     G_CALLBACK(manage_image_list_select),
                     del_bun);

    gtk_widget_show_all(images_dialog);
    gtk_dialog_run(GTK_DIALOG(images_dialog));
    if(GTK_IS_WIDGET(images_dialog))
        gtk_widget_destroy(images_dialog);
    gtk_tree_store_clear(store);
    return ;
}
static void menu_cb_ManageImage(GtkToolButton *toolbutton, void *data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return ;
    }
    g_source_remove(timer);
    
    char *host;

    host = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wgt.hostName_entry));
    url = g_string_new("https://");
    g_string_append(url, host);
    g_string_append(url, ":8443/storage/imagelist?");
    g_string_append(url, "_search=false");
    g_string_append(url, "&nd=1514358456936");
    g_string_append(url, "&rows=1000");
    g_string_append(url, "&page=1");
    g_string_append(url, "&sidx=id");
    g_string_append(url, "&sord=desc");

    pthread_mutex_unlock(&lock);
    g_thread_new("get_image_list", curl_send_get, "get_image_list");
    
    dialog_for_progress(_("正在获取磁盘信息"));
    g_free(host);
}
static void manageImage(void *data)
{
    cJSON *recvInfo=data, *images;
    int status;

    close_progress_dialog(NULL);
    if(NULL == recvInfo)
    {
        dialog_for_message(_("返回信息异常，解析失败!"));
        return ;
    }
    
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        images = cJSON_GetObjectItem(recvInfo, "rows");
        dialog_for_manageImage(images);
        pthread_mutex_unlock(&lock);
    } else {
        
        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }

    if(!IS_DEL)
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
    IS_DEL = FALSE;
    cJSON_Delete(recvInfo);
}
static void delImage(gpointer data)
{
    cJSON *recvInfo = data;
    int status;

    close_progress_dialog(NULL);
    if(NULL == recvInfo)
    {
        dialog_for_message(_("返回信息异常，解析失败!"));
        return ;
    }
    
    status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
    if(0 == status)
    {
        gtk_widget_destroy(images_dialog);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        pthread_mutex_unlock(&lock);
        menu_cb_ManageImage(NULL, NULL);
        pthread_mutex_unlock(&lock);
        
    } else {

        char *error = get_cjson_data(recvInfo, "message");
        dialog_for_message(error);
    }

    cJSON_Delete(recvInfo);
}

static int handle_sock_recvInfo(void *data)
{
    if(0 != pthread_mutex_trylock(&lock))
    {
        dialog_for_message(_("资源繁忙,请稍后再试!"));
        return -1;
    }
    cJSON *recvInfo = cjson_recv;
    char *task=data;
    int status;

    if(NULL != recvInfo)
    {
        status = cJSON_GetObjectItem(recvInfo, "status")->valueint;
        if(!strstr(task, "get_permit") && -6 == status)
        {
            close_progress_dialog(NULL);
            cJSON_Delete(recvInfo);
            pthread_mutex_unlock(&lock);
            dialog_for_message(_("登录异常\n连接已断开!"));
            return 0;
        }
    }
    
printf("handle_sock_recvInfo(): task=%s\n", task);

    if(strstr(task, "get_permit"))  //    get_permit
    {
        login_engine_1(recvInfo);
        pthread_mutex_unlock(&lock);
        return 0;
    }
    if(strstr(task, "post_login"))  //    post_login
    {
        login_engine_2(recvInfo);
        pthread_mutex_unlock(&lock);
        return 0;
    }
    if(strstr(task, "get_logout"))  //     get_logout
    {
        quit_login_1(recvInfo);
        pthread_mutex_unlock(&lock);
        return 0;
    }  
    
    if(strstr(task, "get_all"))  //       get_all
    {
        get_allVmInfo_1(recvInfo);
        pthread_mutex_unlock(&lock);
        return 0;
    }

    if(strstr(task, "post_conn_vm"))  //   post_conn_vm
    {
        connect_vm_1(recvInfo);
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return 0;
    }

    if(strstr(task, "post_new_vm"))  //   post_new_vm
    {
        createVm(recvInfo);
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return 0;
    }

    if(strstr(task, "post_delete"))  //   post_new_vm
    {
        deleteVm(recvInfo);
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return 0;
    } 

    if(strstr(task, "post_edit_vm"))  //   post_edit_vm
    {
        editVm(recvInfo);
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return 0;
    } 
        
    if(strstr(task, "post_start") || strstr(task, "post_runOnce"))  //   post_start && post_runOnce
    {
        startVm(recvInfo);
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return 0;
    }
    
    if(strstr(task, "post_changecd"))  //   post_changecd
    {
        changeCd(recvInfo);
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return 0;
    }
    
    if(strstr(task, "post_take_shot"))  //   post_take_shot
    {
        pause_take_shot(recvInfo);
        pthread_mutex_unlock(&lock);
        return 0;
    }
    if(strstr(task, "post_pause"))  //   post_pause
    {
        pauseVm(recvInfo);
        pthread_mutex_unlock(&lock);
        return 0;
    }  
    
    if(strstr(task, "post_stop"))  //   post_stop
    {
        stopVm(recvInfo);
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return 0;
    } 
    if(strstr(task, "post_restart"))  //   post_restart
    {
        restartVm(recvInfo);
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return 0;
    }
    
    if(strstr(task, "post_save_snap"))  //  post_save_snap
    {
        saveSnapShot(recvInfo);
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return 0;
    }

    if(strstr(task, "get_snap_tree"))  //  get_snap_tree
    {
        manageSnapShot(recvInfo);   
        pthread_mutex_unlock(&lock);
        return 0;
    }
        
    if(strstr(task, "post_re_snap"))  //  post_re_snap
    {
        reSnapShot(recvInfo);
        pthread_mutex_unlock(&lock);
        return 0;
    }
    
    if(strstr(task, "post_del_snap"))  //   post_del_snap
    {
        delSnapShot(recvInfo);
        pthread_mutex_unlock(&lock);
        return 0;
    }

    if(strstr(task, "get_image_list"))  //  get_image_list
    {
        manageImage(recvInfo);
        pthread_mutex_unlock(&lock);
        return 0;
    }

    if(strstr(task, "post_del_image"))  //   post_del_image
    {
        delImage(recvInfo);
        pthread_mutex_unlock(&lock);
        return 0;
    }

    if(strstr(task, "post_update_passwd"))  //   post_update_passwd
    {
        updatePasswd(recvInfo);
        pthread_mutex_unlock(&lock);
        timer = g_timeout_add(15000, (GSourceFunc)deal_time, NULL);
        return 0;
    }
    
    return 0;
}

#ifdef USE_SMARTCARD
static void enable_smartcard_actions(SpiceWindow *win, VReader *reader,
                                     gboolean can_insert, gboolean can_remove)
{
    GtkAction *action;

    if ((reader != NULL) && (!spice_smartcard_reader_is_software((SpiceSmartcardReader*)reader)))
    {
        /* Having menu actions to insert/remove smartcards only makes sense
         * for software smartcard readers, don't do anything when the event
         * we received was for a "real" smartcard reader.
         */
        return;
    }
    action = gtk_action_group_get_action(win->ag, "InsertSmartcard");
    g_return_if_fail(action != NULL);
    gtk_action_set_sensitive(action, can_insert);
    action = gtk_action_group_get_action(win->ag, "RemoveSmartcard");
    g_return_if_fail(action != NULL);
    gtk_action_set_sensitive(action, can_remove);
}


static void reader_added_cb(SpiceSmartcardManager *manager, VReader *reader,
                            gpointer user_data)
{
    enable_smartcard_actions(user_data, reader, TRUE, FALSE);
}

static void reader_removed_cb(SpiceSmartcardManager *manager, VReader *reader,
                              gpointer user_data)
{
    enable_smartcard_actions(user_data, reader, FALSE, FALSE);
}

static void card_inserted_cb(SpiceSmartcardManager *manager, VReader *reader,
                             gpointer user_data)
{
    enable_smartcard_actions(user_data, reader, FALSE, TRUE);
}

static void card_removed_cb(SpiceSmartcardManager *manager, VReader *reader,
                            gpointer user_data)
{
    enable_smartcard_actions(user_data, reader, TRUE, FALSE);
}

static void menu_cb_insert_smartcard(GtkAction *action, void *data)
{
    spice_smartcard_manager_insert_card(spice_smartcard_manager_get());
}

static void menu_cb_remove_smartcard(GtkAction *action, void *data)
{
    spice_smartcard_manager_remove_card(spice_smartcard_manager_get());
}
#endif

#ifdef USE_USBREDIR
static void remove_cb(GtkContainer *container, GtkWidget *widget, void *data)
{
    gtk_window_resize(GTK_WINDOW(data), 1, 1);
}

static void menu_cb_select_usb_devices(GtkToolButton *toolbutton, void *data)
{
    GtkWidget *dialog, *area, *usb_device_widget;
    SpiceWindow *win = data;

    if(NULL == win->conn)
        return ;

    /* Create the widgets */
    dialog = gtk_dialog_new_with_buttons(
                    _("Select USB devices for redirection"),
                    GTK_WINDOW(win->toplevel),
                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT,
                    NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 12);
    gtk_box_set_spacing(GTK_BOX(gtk_bin_get_child(GTK_BIN(dialog))), 12);

    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    usb_device_widget = spice_usb_device_widget_new(win->conn->session,
                                                    NULL); /* default format */
    g_signal_connect(usb_device_widget, "connect-failed",
                     G_CALLBACK(usb_connect_failed), NULL);
    gtk_box_pack_start(GTK_BOX(area), usb_device_widget, TRUE, TRUE, 0);

    /* This shrinks the dialog when USB devices are unplugged */
    g_signal_connect(usb_device_widget, "remove",
                     G_CALLBACK(remove_cb), dialog);

    /* show and run */
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
#endif

static void menu_cb_bool_prop(GtkToggleAction *action, gpointer data)
{
    SpiceWindow *win = data;
    gboolean state = gtk_toggle_action_get_active(action);
    const char *name;
    gpointer object;

    name = gtk_action_get_name(GTK_ACTION(action));
    SPICE_DEBUG("%s: %s = %s", __FUNCTION__, name, state ? _("yes") : _("no"));

    g_key_file_set_boolean(keyfile, "general", name, state);

    if (is_gtk_session_property(name)) {
        object = win->conn->gtk_session;
    } else {
        object = win->spice;
    }
    g_object_set(object, name, state, NULL);
}

static void menu_cb_conn_bool_prop_changed(GObject    *gobject,
                                           GParamSpec *pspec,
                                           gpointer    user_data)
{
    SpiceWindow *win = user_data;
    const gchar *property = g_param_spec_get_name(pspec);
    GtkAction *toggle;
    gboolean state;

    toggle = gtk_action_group_get_action(win->ag, property);
    g_object_get(win->conn->gtk_session, property, &state, NULL);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(toggle), state);
}

static void menu_cb_toolbar(GtkToggleAction *action, gpointer data)
{
    SpiceWindow *win = data;
    gboolean state = gtk_toggle_action_get_active(action);

    gtk_widget_set_visible(win->toolbar, state);
    g_key_file_set_boolean(keyfile, "ui", "toolbar", state);
}

static void menu_cb_statusbar(GtkToggleAction *action, gpointer data)
{
    SpiceWindow *win = data;
    gboolean state = gtk_toggle_action_get_active(action);

    gtk_widget_set_visible(win->statusbar, state);
    g_key_file_set_boolean(keyfile, "ui", "statusbar", state);
}

static void menu_cb_about(GtkAction *action, void *data)
{
    char *comments = _("gtk test client app for the\n"
        "spice remote desktop protocol");
    static const char *copyright = "(c) 2010 Red Hat";
    static const char *website = "http://www.spice-space.org";
    static const char *authors[] = { "Gerd Hoffmann <kraxel@redhat.com>",
                               "Marc-André Lureau <marcandre.lureau@redhat.com>",
                               NULL };
    SpiceWindow *win = data;

    gtk_show_about_dialog(GTK_WINDOW(win->toplevel),
                          "authors",         authors,
                          "comments",        comments,
                          "copyright",       copyright,
                          "logo-icon-name",  GTK_STOCK_ABOUT,
                          "website",         website,
                          "version",         PACKAGE_VERSION,
                          "license",         "LGPLv2.1",
                          NULL);
}

static gboolean delete_cb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    SpiceWindow *win = data;
    conn_info *ci;

    if(connections > 0)
    {
        if(NULL != win->conn)
        {
            win->conn->wins[win->id * CHANNELID_MAX + win->monitor_id] = NULL;     
            win->conn = NULL;
        }
        while(conn_list != NULL)
        {
            ci = conn_list->data;
            g_signal_handlers_block_by_func(ci->channel, G_CALLBACK(main_channel_event), ci->conn);
            g_signal_handlers_block_by_func(ci->channel, G_CALLBACK(main_agent_update), ci->conn);
            connection_disconnect(ci->conn);
            conn_list = g_slist_remove(conn_list, ci);
            g_free(ci->vmName);
            g_free(ci);
        }
    }

    g_object_unref(win->ag);
    g_object_unref(win->ui);
    gtk_widget_destroy(win->toplevel);
    g_object_unref(win); 
    if(IS_LOGIN)
        global_free();  
 
    g_main_loop_quit(mainloop);
    return true;
}

static gboolean window_state_cb(GtkWidget *widget, GdkEventWindowState *event,
                                gpointer data)
{
    SpiceWindow *win = data;
    if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) {
        win->fullscreen = event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN;
        if (win->fullscreen) {
       //     gtk_widget_hide(win->menubar);
            gtk_widget_hide(win->toolbar);
            gtk_widget_hide(win->statusbar);
            gtk_widget_grab_focus(win->spice);
        } else {
            gboolean state;
            GtkAction *toggle;

       //     gtk_widget_show(win->menubar);
            toggle = gtk_action_group_get_action(win->ag, "Toolbar");
            state = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(toggle));
            gtk_widget_set_visible(win->toolbar, state);
            toggle = gtk_action_group_get_action(win->ag, "Statusbar");
            state = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(toggle));
            gtk_widget_set_visible(win->statusbar, state);
        }
    }
    return TRUE;
}

static void grab_keys_pressed_cb(GtkWidget *widget, gpointer data)
{
    SpiceWindow *win = data;

    /* since mnemonics are disabled, we leave fullscreen when
       ungrabbing mouse. Perhaps we should have a different handling
       of fullscreen key, or simply use a UI, like vinagre */
    window_set_fullscreen(win, FALSE);
}

static void mouse_grab_cb(GtkWidget *widget, gint grabbed, gpointer data)
{
    SpiceWindow *win = data;

    win->mouse_grabbed = grabbed;
    update_status(win->conn);
}

static void keyboard_grab_cb(GtkWidget *widget, gint grabbed, gpointer data)
{
    SpiceWindow *win = data;
    GtkSettings *settings = gtk_widget_get_settings (widget);

    if (grabbed) {
        /* disable mnemonics & accels */
        g_object_get(settings,
                     "gtk-enable-accels", &win->enable_accels_save,
                     "gtk-enable-mnemonics", &win->enable_mnemonics_save,
                     NULL);
        g_object_set(settings,
                     "gtk-enable-accels", FALSE,
                     "gtk-enable-mnemonics", FALSE,
                     NULL);
    } else {
        g_object_set(settings,
                     "gtk-enable-accels", win->enable_accels_save,
                     "gtk-enable-mnemonics", win->enable_mnemonics_save,
                     NULL);
    }
}

static void restore_configuration(SpiceWindow *win)
{
    gboolean state;
    gchar *str;
    gchar **keys = NULL;
    gsize nkeys, i;
    GError *error = NULL;
    gpointer object;

    keys = g_key_file_get_keys(keyfile, "general", &nkeys, &error);
    if (error != NULL) {
        if (error->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND)
            g_warning("Failed to read configuration file keys: %s", error->message);
        g_clear_error(&error);
        return;
    }

    if (nkeys > 0)
        g_return_if_fail(keys != NULL);

    for (i = 0; i < nkeys; ++i) {
        if (g_str_equal(keys[i], "grab-sequence"))
            continue;
        state = g_key_file_get_boolean(keyfile, "general", keys[i], &error);
        if (error != NULL) {
            g_clear_error(&error);
            continue;
        }

        if (is_gtk_session_property(keys[i])) {
            object = win->conn->gtk_session;
        } else {
            object = win->spice;
        }
        g_object_set(object, keys[i], state, NULL);
    }

    g_strfreev(keys);

    str = g_key_file_get_string(keyfile, "general", "grab-sequence", &error);
    if (error == NULL) {
        SpiceGrabSequence *seq = spice_grab_sequence_new_from_string(str);
        spice_display_set_grab_keys(SPICE_DISPLAY(win->spice), seq);
        spice_grab_sequence_free(seq);
        g_free(str);
    }
    g_clear_error(&error);


    state = g_key_file_get_boolean(keyfile, "ui", "toolbar", &error);
    if (error == NULL)
        gtk_widget_set_visible(win->toolbar, state);
    g_clear_error(&error);

    state = g_key_file_get_boolean(keyfile, "ui", "statusbar", &error);
    if (error == NULL)
        gtk_widget_set_visible(win->statusbar, state);
    g_clear_error(&error);
}

/* ------------------------------------------------------------------ */

static const GtkActionEntry entries[] = {
    {
        .name        = "FileMenu",
        .label       = "_File",
    },{
        .name        = "FileRecentMenu",
        .label       = "_Recent",
    },{
        .name        = "EditMenu",
        .label       = "_Edit",
    },{
        .name        = "ViewMenu",
        .label       = "_View",
    },{
        .name        = "InputMenu",
        .label       = "_Input",
    },{
        .name        = "OptionMenu",
        .label       = "_Options",
    },{
        .name        = "HelpMenu",
        .label       = "_Help",
    },{

        /* File menu */
        .name        = "Connect",
//        .stock_id    = GTK_STOCK_CONNECT,
        .label       = N_("_Connect ..."),
        .callback    = G_CALLBACK(menu_cb_connect),
    },{
        .name        = "Close",
        .stock_id    = GTK_STOCK_CLOSE,
        .label       = N_("_Close"),
        .callback    = G_CALLBACK(menu_cb_close),
        .accelerator = "", /* none (disable default "<control>W") */
    },{

        /* Edit menu */
        .name        = "CopyToGuest",
        .stock_id    = GTK_STOCK_COPY,
        .label       = N_("_Copy to guest"),
        .callback    = G_CALLBACK(menu_cb_copy),
        .accelerator = "", /* none (disable default "<control>C") */
    },{
        .name        = "PasteFromGuest",
        .stock_id    = GTK_STOCK_PASTE,
        .label       = N_("_Paste from guest"),
        .callback    = G_CALLBACK(menu_cb_paste),
        .accelerator = "", /* none (disable default "<control>V") */
    },{

        .name        = "Fullscreen",
        .stock_id    = GTK_STOCK_FULLSCREEN,
        .label       = N_("_Fullscreen"),
        .callback    = G_CALLBACK(menu_cb_fullscreen),
        .accelerator = "<shift>F11",
        .tooltip     = N_("全屏"),
    },{

        .name        = "CreateVm",
        .stock_id    = GTK_STOCK_ADD,
        .label       = N_("_CreateVm"),
        .callback    = G_CALLBACK(menu_cb_CreateVm),
        .accelerator = "",
        .tooltip     = N_("新建虚拟机"),
    },{
        .name        = "EditVm",
        .stock_id    = GTK_STOCK_PREFERENCES,
        .label       = N_("_EditVm"),
        .callback    = G_CALLBACK(menu_cb_EditVm),
        .accelerator = "",
        .tooltip     = N_("编辑虚拟机"),
    },{
        .name        = "DeleteVm",
        .stock_id    = GTK_STOCK_DELETE,
        .label       = N_("_DeleteVm"),
        .callback    = G_CALLBACK(menu_cb_DeleteVm),
        .accelerator = "",
        .tooltip     = N_("删除虚拟机"),
    },{
        .name        = "VmRunOnce",
        .stock_id    = GTK_STOCK_GO_UP,
        .label       = N_("_VmRunOnce"),
        .callback    = G_CALLBACK(menu_cb_RunOnce),
        .accelerator = "",
        .tooltip     = N_("运行一次"),
    },{
        .name        = "ChangeCd",
        .stock_id    = GTK_STOCK_CDROM,   //GTK_STOCK_FIND_AND_REPLACE
        .label       = N_("_ChangeCd"),
        .callback    = G_CALLBACK(menu_cb_ChangeCd),
        .accelerator = "",
        .tooltip     = N_("切换CD"),
    },{

        .name        = "VmStart",
        .stock_id    = GTK_STOCK_MEDIA_PLAY,
        .label       = N_("_VmStart"),
        .callback    = G_CALLBACK(menu_cb_StartVm),
        .accelerator = "",
        .tooltip     = N_("启动虚拟机"),
    },{

        .name        = "VmPause",
        .stock_id    = GTK_STOCK_MEDIA_PAUSE,
        .label       = N_("_VmPause"),
        .callback    = G_CALLBACK(menu_cb_PauseVm),
        .accelerator = "",
        .tooltip     = N_("暂停虚拟机"),
    },{

        .name        = "VmStop",
        .stock_id    = GTK_STOCK_MEDIA_STOP,
        .label       = N_("_VmStop"),
        .callback    = G_CALLBACK(menu_cb_StopVm),
        .accelerator = "",
        .tooltip     = N_("关闭虚拟机"),
    },{

        .name        = "VmRestart",
        .stock_id    = GTK_STOCK_REFRESH,
        .label       = N_("_VmRestart"),
        .callback    = G_CALLBACK(menu_cb_RestartVm),
        .accelerator = "",
        .tooltip     = N_("重启虚拟机"),
    },{

        .name        = "SaveSnapShot",
        .stock_id    = GTK_STOCK_SAVE,
        .label       = N_("_SaveSnapShot"),
        .callback    = G_CALLBACK(menu_cb_SaveSnapShot),
        .accelerator = "",
        .tooltip     = N_("创建快照"),
    },{

        .name        = "ManageSnapShot",
        .stock_id    = GTK_STOCK_ABOUT,
        .label       = N_("_ManageSnapShot"),
        .callback    = G_CALLBACK(menu_cb_ManageSnapShot),
        .accelerator = "",
        .tooltip     = N_("快照管理"),
    },{

        .name        = "ManageImage",
        .stock_id    = GTK_STOCK_HARDDISK,
        .label       = N_("_ManageImage"),
        .callback    = G_CALLBACK(menu_cb_ManageImage),
        .accelerator = "",
        .tooltip     = N_("磁盘管理"),
    },{

        .name        = "UpdatePasswd",
        .stock_id    = GTK_STOCK_COLOR_PICKER,
        .label       = N_("_UpdatePasswd"),
        .callback    = G_CALLBACK(menu_cb_UpdatePasswd),
        .accelerator = "",
        .tooltip     = N_("修改用户密码"),
    },{

#ifdef USE_SMARTCARD
	    .name        = "InsertSmartcard",
	    .label       = N_("_Insert Smartcard"),
	    .callback    = G_CALLBACK(menu_cb_insert_smartcard),
        .accelerator = "<shift>F8",
    },{
	    .name        = "RemoveSmartcard",
	    .label       = N_("_Remove Smartcard"),
	    .callback    = G_CALLBACK(menu_cb_remove_smartcard),
        .accelerator = "<shift>F9",
    },{
#endif

#ifdef USE_USBREDIR
        .name        = "SelectUsbDevices",
        .stock_id    = GTK_STOCK_CONNECT,
        .label       = N_("_设备重定向"),
        .callback    = G_CALLBACK(menu_cb_select_usb_devices),
        .accelerator = "<shift>F10",
        .tooltip     = N_("设备重定向"),
    },{
#endif
        /* Help menu */
        .name        = "About",
        .stock_id    = GTK_STOCK_ABOUT,
        .label       = N_("_About ..."),
        .callback    = G_CALLBACK(menu_cb_about),
    }
};

static const char *spice_display_properties[] = {
    "grab-keyboard",
    "grab-mouse",
    "resize-guest",
    "scaling",
    "disable-inputs",
};

static const char *spice_gtk_session_properties[] = {
    "auto-clipboard",
    "auto-usbredir",
};

static const GtkToggleActionEntry tentries[] = {
    {
        .name        = "grab-keyboard",
        .label       = N_("Grab keyboard when active and focused"),
        .callback    = G_CALLBACK(menu_cb_bool_prop),
    },{
        .name        = "grab-mouse",
        .label       = N_("Grab mouse in server mode (no tabled/vdagent)"),
        .callback    = G_CALLBACK(menu_cb_bool_prop),
    },{
        .name        = "resize-guest",
        .label       = N_("Resize guest to match window size"),
        .callback    = G_CALLBACK(menu_cb_bool_prop),
    },{
        .name        = "scaling",
        .label       = N_("Scale display"),
        .callback    = G_CALLBACK(menu_cb_bool_prop),
    },{
        .name        = "disable-inputs",
        .label       = N_("Disable inputs"),
        .callback    = G_CALLBACK(menu_cb_bool_prop),
    },{
        .name        = "auto-clipboard",
        .label       = N_("Automagic clipboard sharing between host and guest"),
        .callback    = G_CALLBACK(menu_cb_bool_prop),
    },{
        .name        = "auto-usbredir",
        .label       = N_("Auto redirect newly plugged in USB devices"),
        .callback    = G_CALLBACK(menu_cb_bool_prop),
    },{
        .name        = "Statusbar",
        .label       = N_("Statusbar"),
        .callback    = G_CALLBACK(menu_cb_statusbar),
    },{
        .name        = "Toolbar",
        .label       = N_("Toolbar"),
        .callback    = G_CALLBACK(menu_cb_toolbar),
    }
};

static char ui_xml[] =
"<ui>\n"
"  <menubar action='MainMenu'>\n"
"    <menu action='FileMenu'>\n"
"      <menuitem action='Connect'/>\n"
"      <menu action='FileRecentMenu'/>\n"
"      <separator/>\n"
"      <menuitem action='Close'/>\n"
"    </menu>\n"
"    <menu action='EditMenu'>\n"
"      <menuitem action='CopyToGuest'/>\n"
"      <menuitem action='PasteFromGuest'/>\n"
"    </menu>\n"
"    <menu action='ViewMenu'>\n"
"      <menuitem action='Fullscreen'/>\n"
"      <menuitem action='Toolbar'/>\n"
"      <menuitem action='Statusbar'/>\n"
"    </menu>\n"
"    <menu action='InputMenu'>\n"
#ifdef USE_SMARTCARD
"      <menuitem action='InsertSmartcard'/>\n"
"      <menuitem action='RemoveSmartcard'/>\n"
#endif
#ifdef USE_USBREDIR
"      <menuitem action='SelectUsbDevices'/>\n"
#endif
"    </menu>\n"
"    <menu action='OptionMenu'>\n"
"      <menuitem action='grab-keyboard'/>\n"
"      <menuitem action='grab-mouse'/>\n"
"      <menuitem action='resize-guest'/>\n"
"      <menuitem action='scaling'/>\n"
"      <menuitem action='disable-inputs'/>\n"
"      <menuitem action='auto-clipboard'/>\n"
"      <menuitem action='auto-usbredir'/>\n"
"    </menu>\n"
"    <menu action='HelpMenu'>\n"
"      <menuitem action='About'/>\n"
"    </menu>\n"
"  </menubar>\n"  
"  <toolbar action='ToolBar'>\n"
"    <separator/>\n"
"    <toolitem action='Fullscreen'/>\n"
"    <separator/>\n"
"    <toolitem action='SelectUsbDevices'/>\n"
"    <separator/>\n"
"    <toolitem action='CopyToGuest'/>\n"
"    <toolitem action='PasteFromGuest'/>\n"
"    <separator/>\n"
"    <toolitem action='CreateVm'/>\n"
"    <toolitem action='EditVm'/>\n"
"    <toolitem action='DeleteVm'/>\n"
"    <separator/>\n"
"    <toolitem action='VmRunOnce'/>\n"
"    <toolitem action='ChangeCd'/>\n"
"    <separator/>\n"
"    <toolitem action='VmStart'/>\n"
"    <toolitem action='VmPause'/>\n"
"    <toolitem action='VmStop'/>\n"
"    <toolitem action='VmRestart'/>\n"
"    <separator/>\n"
"    <toolitem action='SaveSnapShot'/>\n"
"    <toolitem action='ManageSnapShot'/>\n"
"    <separator/>\n"
"    <toolitem action='ManageImage'/>\n"
"    <separator/>\n"
"    <toolitem action='UpdatePasswd'/>\n"
"    <separator/>\n"
"  </toolbar>\n"
"</ui>\n";

static gboolean is_gtk_session_property(const gchar *property)
{
    int i;

    for (i = 0; i < G_N_ELEMENTS(spice_gtk_session_properties); i++) {
        if (!strcmp(spice_gtk_session_properties[i], property)) {
            return TRUE;
        }
    }
    return FALSE;
}

#ifndef G_OS_WIN32
static void recent_item_activated_cb(GtkRecentChooser *chooser, gpointer data)
{
    GtkRecentInfo *info;
    struct spice_connection *conn;
    const char *uri;

    info = gtk_recent_chooser_get_current_item(chooser);

    uri = gtk_recent_info_get_uri(info);
    g_return_if_fail(uri != NULL);

    conn = connection_new();
    g_object_set(conn->session, "uri", uri, NULL);
    gtk_recent_info_unref(info);
    connection_connect(conn);
}
#endif

static gboolean configure_event_cb(GtkWidget         *widget,
                                   GdkEventConfigure *event,
                                   gpointer           data)
{
    gboolean resize_guest;
    SpiceWindow *win = data;
    if(NULL == win->conn)
        return FALSE;
    g_return_val_if_fail(win != NULL, FALSE);
    g_return_val_if_fail(win->conn != NULL, FALSE);

    g_object_get(win->spice, "resize-guest", &resize_guest, NULL);
    if (resize_guest && win->conn->agent_connected)
        return FALSE;

    return FALSE;
}

static void
spice_window_class_init (SpiceWindowClass *klass)
{
}

static void
spice_window_init (SpiceWindow *self)
{
}

static void
vm_list_selection_changed(GtkTreeSelection* selection, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel* model;
    char *vm;
    int index, status, status1;
    GtkWidget *window;
    
    if(gtk_tree_selection_get_selected(selection, &model, &iter))
    {   
        gtk_tree_model_get(model, &iter, 1, &vm, -1);
        if(0 == g_strcmp0(vm, "Computers"))
        {
            g_free(vm);
            return ;
        }

        if(0 == g_strcmp0(vm, "Origin")) {
            
            if(IS_LOGIN)
            {   
                gtk_widget_show(gtk_notebook_get_nth_page(GTK_NOTEBOOK(win->vmbar), 1));
                gtk_notebook_set_current_page(GTK_NOTEBOOK(win->vmbar), 1);
            }
            else
            {
                gtk_widget_show(gtk_notebook_get_nth_page(GTK_NOTEBOOK(win->vmbar), 0));
                gtk_notebook_set_current_page(GTK_NOTEBOOK(win->vmbar), 0);
            }
            
        } else {
            
            index = get_vmbar_index(vm);
            status = get_vm_status(vm);
            GThread *td;
            pthread_mutex_unlock(&lock);
            td = g_thread_new("vm_id", get_vm_info, vm);
            g_thread_join(td);
            if(NULL == cjson_recv)
            {
                g_free(vm);
                dialog_for_message(_("获取虚拟机信息失败!"));
                return ;
            }
            status1 = cJSON_GetObjectItem(cjson_recv, "status")->valueint;
            if(0 != status1)
            {
                g_free(vm);
                cJSON_Delete(cjson_recv); 
                if(-6 == status1)
                    dialog_for_message(_("登录异常\n连接已断开!"));
                else
                    dialog_for_message(_("获取虚拟机信息失败!"));
                return ;
            }
            cJSON_Delete(cjson_recv);

            if(index > 0 )
            {     
                gtk_notebook_set_current_page(GTK_NOTEBOOK(win->vmbar), index);
  
            } else {

                if(status > 0 && status < 3)
                {   
                    
                    connect_vm(vm);   
                    
                } else {
                    
                    window = vm_set_diswindow(vm, status); 
                    if(NULL == window)
                    {   
                        g_free(vm);
                        dialog_for_message(_("获取虚拟机信息失败!"));
                        return ;
                    }
                    gtk_widget_show_all(window);
                    gtk_notebook_insert_page(GTK_NOTEBOOK(win->vmbar), window, linx_noetbook_tab_label(vm), 2);
                    gtk_notebook_set_current_page(GTK_NOTEBOOK(win->vmbar), 2);
                }
            }
        }
    }
    
    g_free(vm);
}

static GtkWidget* init_tree(void)
{
    GtkWidget *tree;
    GtkTreeViewColumn *column, *column2;
    GtkCellRenderer *renderer_txt, *renderer_pixbuf;
    GtkTreeStore *store;
    GtkTreeIter iter;
    GtkTreeSelection *select;

    store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, GTK_STOCK_HOME, 1, "Origin", -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, GTK_STOCK_PRINT_PREVIEW, 1, "Computers", -1);
    
    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

    renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
    renderer_txt = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer_txt), "foreground", "black", NULL);
    //g_object_set(G_OBJECT(renderer_txt), "editable", TRUE, NULL);
    column = gtk_tree_view_column_new_with_attributes("",
                                                       renderer_pixbuf,
                                                       "stock-id",
                                                       0,
                                                       NULL);
    column2 = gtk_tree_view_column_new_with_attributes("",
                                                       renderer_txt,
                                                       "text",
                                                       1,
                                                       NULL);
    gtk_tree_view_column_set_resizable(column2, TRUE);

    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
    g_signal_connect(G_OBJECT(select),
                     "changed",
                     G_CALLBACK(vm_list_selection_changed),
                     NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column2);

    g_object_unref(store);
    return tree;
}

static GtkWidget* init_toolbar(void)
{
    GtkWidget *toolbar;
    GtkWidget *full, *usb, *add, *edit, *del, *once, *cd, *start,
                *pause, *stop, *restart, *savesnap, *mansnap, *manimage, *user;
    GtkToolItem *full1, *usb1, *add1, *edit1, *del1, *once1, *cd1, *start1,
                *pause1, *stop1, *restart1, *savesnap1, *mansnap1, *manimage1, *user1;
    GtkToolItem *sep, *sep1, *sep2, *sep3, *sep4, *sep5, *sep6, *sep7;
    
    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
    sep = gtk_separator_tool_item_new();
    sep1 = gtk_separator_tool_item_new();
    sep2 = gtk_separator_tool_item_new();
    sep3 = gtk_separator_tool_item_new();
    sep4 = gtk_separator_tool_item_new();
    sep5 = gtk_separator_tool_item_new();
    sep6 = gtk_separator_tool_item_new();
    sep7 = gtk_separator_tool_item_new();

    full = gtk_image_new_from_file("/etc/linx/ppms/png/full.png");    
    full1 = gtk_tool_button_new(full, NULL);
    usb = gtk_image_new_from_file("/etc/linx/ppms/png/usb.png");    
    usb1 = gtk_tool_button_new(usb, NULL);
    add = gtk_image_new_from_file("/etc/linx/ppms/png/add.png");    
    add1 = gtk_tool_button_new(add, NULL);
    edit = gtk_image_new_from_file("/etc/linx/ppms/png/edit.png");    
    edit1 = gtk_tool_button_new(edit, NULL);
    del = gtk_image_new_from_file("/etc/linx/ppms/png/del.png");    
    del1 = gtk_tool_button_new(del, NULL);
    once = gtk_image_new_from_file("/etc/linx/ppms/png/once.png");    
    once1 = gtk_tool_button_new(once, NULL);
    cd = gtk_image_new_from_file("/etc/linx/ppms/png/cd.png");    
    cd1 = gtk_tool_button_new(cd, NULL);
    start = gtk_image_new_from_file("/etc/linx/ppms/png/start.png");    
    start1 = gtk_tool_button_new(start, NULL);
    pause = gtk_image_new_from_file("/etc/linx/ppms/png/pause.png");    
    pause1 = gtk_tool_button_new(pause, NULL);
    stop = gtk_image_new_from_file("/etc/linx/ppms/png/stop.png");    
    stop1 = gtk_tool_button_new(stop, NULL);
    restart = gtk_image_new_from_file("/etc/linx/ppms/png/restart.png");    
    restart1 = gtk_tool_button_new(restart, NULL);
    savesnap = gtk_image_new_from_file("/etc/linx/ppms/png/savesnap.png");    
    savesnap1 = gtk_tool_button_new(savesnap, NULL);
    mansnap = gtk_image_new_from_file("/etc/linx/ppms/png/mansnap.png");    
    mansnap1 = gtk_tool_button_new(mansnap, NULL);
    manimage = gtk_image_new_from_file("/etc/linx/ppms/png/manimage.png");    
    manimage1 = gtk_tool_button_new(manimage, NULL);
    user = gtk_image_new_from_file("/etc/linx/ppms/png/user.png");    
    user1 = gtk_tool_button_new(user, NULL);

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), full1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), usb1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), add1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), edit1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), del1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep2, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), once1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), cd1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep3, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), start1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), pause1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), stop1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), restart1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep4, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), savesnap1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), mansnap1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep5, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), manimage1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep6, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), user1, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep7, -1);

    gtk_tool_item_set_tooltip_text(full1, _("全屏"));
    gtk_tool_item_set_tooltip_text(usb1, _("设备重定向"));
    gtk_tool_item_set_tooltip_text(add1, _("新建虚拟机"));
    gtk_tool_item_set_tooltip_text(edit1, _("编辑虚拟机"));
    gtk_tool_item_set_tooltip_text(del1, _("删除虚拟机"));
    gtk_tool_item_set_tooltip_text(once1, _("运行一次"));
    gtk_tool_item_set_tooltip_text(cd1, _("切换CD"));
    gtk_tool_item_set_tooltip_text(start1, _("启动虚拟机"));
    gtk_tool_item_set_tooltip_text(pause1, _("暂停虚拟机"));
    gtk_tool_item_set_tooltip_text(stop1, _("关闭虚拟机"));
    gtk_tool_item_set_tooltip_text(restart1, _("重启虚拟机"));
    gtk_tool_item_set_tooltip_text(savesnap1, _("创建快照"));
    gtk_tool_item_set_tooltip_text(mansnap1, _("快照管理"));
    gtk_tool_item_set_tooltip_text(manimage1, _("磁盘管理"));
    gtk_tool_item_set_tooltip_text(user1, _("修改用户密码"));

    g_signal_connect(full1, "clicked",
                         G_CALLBACK(menu_cb_fullscreen), win);
    g_signal_connect(usb1, "clicked",
                         G_CALLBACK(menu_cb_select_usb_devices), win);
    g_signal_connect(add1, "clicked",
                         G_CALLBACK(menu_cb_CreateVm), NULL);
    g_signal_connect(edit1, "clicked",
                         G_CALLBACK(menu_cb_EditVm), NULL);
    g_signal_connect(del1, "clicked",
                         G_CALLBACK(menu_cb_DeleteVm), NULL);
    g_signal_connect(once1, "clicked",
                         G_CALLBACK(menu_cb_RunOnce), NULL);
    g_signal_connect(cd1, "clicked",
                         G_CALLBACK(menu_cb_ChangeCd), NULL);
    g_signal_connect(start1, "clicked",
                         G_CALLBACK(menu_cb_StartVm), NULL);
    g_signal_connect(pause1, "clicked",
                         G_CALLBACK(menu_cb_PauseVm), NULL);
    g_signal_connect(stop1, "clicked",
                         G_CALLBACK(menu_cb_StopVm), NULL);
    g_signal_connect(restart1, "clicked",
                         G_CALLBACK(menu_cb_RestartVm), NULL);
    g_signal_connect(savesnap1, "clicked",
                         G_CALLBACK(menu_cb_SaveSnapShot), NULL);
    g_signal_connect(mansnap1, "clicked",
                         G_CALLBACK(menu_cb_ManageSnapShot), NULL);
    g_signal_connect(manimage1, "clicked",
                         G_CALLBACK(menu_cb_ManageImage), NULL);
    g_signal_connect(user1, "clicked",
                         G_CALLBACK(menu_cb_UpdatePasswd), NULL);

    return toolbar;
}

static void destroy_spice_window(SpiceWindow *win)
{
    if (win == NULL)
        return;

    SPICE_DEBUG("destroy window (#%d:%d)", win->id, win->monitor_id);
    g_object_unref(win->ag);
    g_object_unref(win->ui);
    gtk_widget_destroy(win->toplevel);
    g_object_unref(win);
}

/* ------------------------------------------------------------------ */

static void recent_add(SpiceSession *session)
{
    GtkRecentManager *recent;
    GtkRecentData meta = {
        .mime_type    = (char*)"application/x-spice",
        .app_name     = (char*)"spicy",
        .app_exec     = (char*)"spicy --uri=%u",
    };
    char *uri;

    g_object_get(session, "uri", &uri, NULL);
    SPICE_DEBUG("%s: %s", __FUNCTION__, uri);

    recent = gtk_recent_manager_get_default();
    if (g_str_has_prefix(uri, "spice://"))
        meta.display_name = uri + 8;
    else if (g_str_has_prefix(uri, "spice+unix://"))
        meta.display_name = uri + 13;
    else
        g_return_if_reached();

    if (!gtk_recent_manager_add_full(recent, uri, &meta))
        g_warning("Recent item couldn't be added successfully");

    g_free(uri);
}

static void main_channel_event(SpiceChannel *channel, SpiceChannelEvent event,
                               gpointer data)
{
  //  printf("main_channel_event:  123\n");
    const GError *error = NULL;
    spice_connection *conn = data;
    char password[64];
    int rc;
    
    switch (event) {
    case SPICE_CHANNEL_OPENED:
        g_message("main channel: opened");
        recent_add(conn->session);
        break;
    case SPICE_CHANNEL_SWITCHING:
        g_message("main channel: switching host");
        break;
    case SPICE_CHANNEL_CLOSED:
        /* this event is only sent if the channel was succesfully opened before */ 

        if(!IS_LOGIN)
            return ;
        g_message("main channel: closed"); 
        IS_VM_PAUSE = FALSE;
        while_conn_shutdown(conn);

        printf("while_conn_shutdown end **************************!!!!\n\n");

        break;
    case SPICE_CHANNEL_ERROR_IO:
        connection_disconnect(conn);
        break;
    case SPICE_CHANNEL_ERROR_TLS:
    case SPICE_CHANNEL_ERROR_LINK:
    case SPICE_CHANNEL_ERROR_CONNECT:
        error = spice_channel_get_error(channel);
        g_message("main channel: failed to connect");
        if (error) {
            g_message("channel error: %s", error->message);
        }
        connections--;    
            
/*        rc = connect_dialog(conn->session);
        if (rc == 0) {
            connection_connect(conn);
        } else {
            connection_disconnect(conn);
        } */ 
        break;
    case SPICE_CHANNEL_ERROR_AUTH:
        g_warning("main channel: auth failure (wrong password?)");
        strcpy(password, "");
        /* FIXME i18 */
        rc = ask_user(NULL, _("Authentication"),
                      _("Please enter the spice server password"),
                      password, sizeof(password), true);
        if (rc == 0) {
            g_object_set(conn->session, "password", password, NULL);
            connection_connect(conn);
        } else {
            connection_disconnect(conn);
        }
        break;
    default:
        /* TODO: more sophisticated error handling */
        g_warning("unknown main channel event: %d", event);
        /* connection_disconnect(conn); */
        break;
    }
}

static void main_mouse_update(SpiceChannel *channel, gpointer data)
{
    spice_connection *conn = data;
    gint mode;
    
//printf("main_mouse_update():\n");
    
    g_object_get(channel, "mouse-mode", &mode, NULL);
    switch (mode) {
    case SPICE_MOUSE_MODE_SERVER:
        conn->mouse_state = "server";
        break;
    case SPICE_MOUSE_MODE_CLIENT:
        conn->mouse_state = "client";
        break;
    default:
        conn->mouse_state = "?";
        break;
    }
    update_status(conn);
}

static void main_agent_update(SpiceChannel *channel, gpointer data)
{
    spice_connection *conn = data;
    
    g_object_get(channel, "agent-connected", &conn->agent_connected, NULL);
    conn->agent_state = conn->agent_connected ? _("yes") : _("no");
    update_status(conn);
    update_edit_menu(conn);
}

static void inputs_modifiers(SpiceChannel *channel, gpointer data)
{
    spice_connection *conn = data;
    int m, i;

    g_object_get(channel, "key-modifiers", &m, NULL);
    for (i = 0; i < SPICE_N_ELEMENTS(conn->wins); i++) {
        if (conn->wins[i] == NULL)
            continue;

        gtk_label_set_text(GTK_LABEL(conn->wins[i]->st[STATE_SCROLL_LOCK]),
                           m & SPICE_KEYBOARD_MODIFIER_FLAGS_SCROLL_LOCK ? _("SCROLL") : "");
        gtk_label_set_text(GTK_LABEL(conn->wins[i]->st[STATE_CAPS_LOCK]),
                           m & SPICE_KEYBOARD_MODIFIER_FLAGS_CAPS_LOCK ? _("CAPS") : "");
        gtk_label_set_text(GTK_LABEL(conn->wins[i]->st[STATE_NUM_LOCK]),
                           m & SPICE_KEYBOARD_MODIFIER_FLAGS_NUM_LOCK ? _("NUM") : "");
    }
}

static void display_mark(SpiceChannel *channel, gint mark, SpiceWindow *win)
{
    g_return_if_fail(win != NULL);
    g_return_if_fail(win->toplevel != NULL);

    if (mark == TRUE) {
        gtk_widget_show(win->toplevel);
    } else {
        gtk_widget_hide(win->toplevel);
    }
}

static void update_auto_usbredir_sensitive(spice_connection *conn)
{
#ifdef USE_USBREDIR
    int i;
    GtkAction *ac;
    gboolean sensitive;

    sensitive = spice_session_has_channel_type(conn->session,
                                               SPICE_CHANNEL_USBREDIR);
    for (i = 0; i < SPICE_N_ELEMENTS(conn->wins); i++) {
        if (conn->wins[i] == NULL)
            continue;
        ac = gtk_action_group_get_action(conn->wins[i]->ag, "auto-usbredir");
        gtk_action_set_sensitive(ac, sensitive);
    }
#endif
}

static SpiceWindow* get_window(spice_connection *conn, int channel_id, int monitor_id)
{
    g_return_val_if_fail(channel_id < CHANNELID_MAX, NULL);
    g_return_val_if_fail(monitor_id < MONITORID_MAX, NULL);

    return conn->wins[channel_id * CHANNELID_MAX + monitor_id];
}

static void add_window(spice_connection *conn, SpiceWindow *win)
{
    g_return_if_fail(win != NULL);
    g_return_if_fail(win->id < CHANNELID_MAX);
    g_return_if_fail(win->monitor_id < MONITORID_MAX);
    g_return_if_fail(conn->wins[win->id * CHANNELID_MAX + win->monitor_id] == NULL);

    SPICE_DEBUG("add display monitor %d:%d", win->id, win->monitor_id);
    conn->wins[win->id * CHANNELID_MAX + win->monitor_id] = win;
}

static void del_window(spice_connection *conn, SpiceWindow *win)
{
    if (win == NULL)
        return;

    g_return_if_fail(win->id < CHANNELID_MAX);
    g_return_if_fail(win->monitor_id < MONITORID_MAX);

    g_debug("del display monitor %d:%d", win->id, win->monitor_id);
    conn->wins[win->id * CHANNELID_MAX + win->monitor_id] = NULL;
    if (win->id > 0)
        spice_main_set_display_enabled(conn->main, win->id, FALSE);
    else
        spice_main_set_display_enabled(conn->main, win->monitor_id, FALSE);
    spice_main_send_monitor_config(conn->main);
       
    destroy_spice_window(win);
}


static void display_monitors(SpiceChannel *display, GParamSpec *pspec,
                             spice_connection *conn)          //lfx3
{
    printf("display_monitors: sss \n");

    GArray *monitors=NULL;
    int id, index;
    guint i;
    conn_info *ci=NULL, *cn=NULL;
    char *vmName=NULL, *vm=NULL;
    GtkWidget *spice;

    cn = get_conn_from_list_by_conn(conn);
    g_object_get(display, "channel-id", &id, "monitors", &monitors, NULL);
    g_return_if_fail(monitors != NULL);

    spice = GTK_WIDGET(spice_display_new_with_monitor(conn->session, id, 0));
    SpiceDisplay *sd = SPICE_DISPLAY(spice);
    SpiceDisplayPrivate *sdp = sd->priv;
    printf("w= %d   h= %d\n\n", sdp->area.width, sdp->area.height);
    if(NULL != cn)
    {
        if(conn == win->conn)
        {
            index = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->vmbar));
            if(index < 2)
                return ;
            vm = get_vmbar_vmname(index);
            if(0 == strcmp(vm, cn->vmName))
            {
                gtk_widget_set_size_request(win->toplevel, sdp->area.width+164, sdp->area.height+99);
                gtk_window_resize(GTK_WINDOW(win->toplevel), sdp->area.width+164, sdp->area.height+99);
                gtk_widget_queue_resize_no_redraw(win->toplevel);
            }
        }
        return ;
    }

    if(NULL == (ci = (conn_info *)malloc(sizeof(struct conn_info))))
    {
        g_print("malloc failed!\n"); 
        return ;
    }
    if(NULL == (vmName = (char *)malloc(sizeof(current_conn_vm))))
    { 
        g_print("malloc failed!\n"); 
        return ;
    }
    for (i = 0; i < monitors->len; i++) { 
        SpiceWindow *w;

        if (!get_window(conn, id, i)) {
            //GtkWidget *spice = GTK_WIDGET(spice_display_new_with_monitor(conn->session, id, i));

            strcpy(vmName, current_conn_vm);
            ci->vmName = vmName;
            ci->conn = conn;
            ci->display_channel = display;
            ci->id = id;
            ci->monitor_id = i;
            ci->spice = spice;
            ci->channel = current_channel;

            if(win->conn != NULL)
            {   
                win->conn->wins[win->id * CHANNELID_MAX + win->monitor_id] = NULL;
                g_signal_handlers_block_by_func(win->spice, G_CALLBACK(configure_event_cb), win);
                g_signal_handlers_block_by_func(win->spice, G_CALLBACK(mouse_grab_cb), win);
                g_signal_handlers_block_by_func(win->spice, G_CALLBACK(keyboard_grab_cb), win);
                g_signal_handlers_block_by_func(win->spice, G_CALLBACK(grab_keys_pressed_cb), win);
            }
            g_signal_handlers_block_by_func(win->vmbar, G_CALLBACK(vmbar_switch_page), win);
            w = update_spice_window(ci);
            g_signal_handlers_unblock_by_func(win->vmbar, G_CALLBACK(vmbar_switch_page), win);
            add_window(conn, w);
            spice_g_signal_connect_object(display, "display-mark",
                                         G_CALLBACK(display_mark), w, 0);
            update_auto_usbredir_sensitive(conn);
            conn_list = g_slist_append(conn_list, ci);
        }
    }

    for (; i < MONITORID_MAX; i++)
        del_window(conn, get_window(conn, id, i));
    g_clear_pointer(&monitors, g_array_unref);

    gtk_widget_set_size_request(win->toplevel, sdp->area.width+164, sdp->area.height+99);
    gtk_window_resize(GTK_WINDOW(win->toplevel), sdp->area.width+164, sdp->area.height+99);
    gtk_widget_queue_resize_no_redraw(win->toplevel);

    update_toolbar_status(1);
}

static void port_write_cb(GObject *source_object,
                          GAsyncResult *res,
                          gpointer user_data)
{
    SpicePortChannel *port = SPICE_PORT_CHANNEL(source_object);
    GError *error = NULL;

    spice_port_write_finish(port, res, &error);
    if (error != NULL)
        g_warning("%s", error->message);
    g_clear_error(&error);
}

static void port_flushed_cb(GObject *source_object,
                            GAsyncResult *res,
                            gpointer user_data)
{
    SpiceChannel *channel = SPICE_CHANNEL(source_object);
    GError *error = NULL;

    spice_channel_flush_finish(channel, res, &error);
    if (error != NULL)
        g_warning("%s", error->message);
    g_clear_error(&error);

    spice_channel_disconnect(channel, SPICE_CHANNEL_CLOSED);
}

static gboolean input_cb(GIOChannel *gin, GIOCondition condition, gpointer data)
{
    char buf[4096];
    gsize bytes_read;
    GIOStatus status;

    if (!(condition & G_IO_IN))
        return FALSE;

    status = g_io_channel_read_chars(gin, buf, sizeof(buf), &bytes_read, NULL);
    if (status != G_IO_STATUS_NORMAL)
        return FALSE;

    if (stdin_port != NULL)
        spice_port_write_async(stdin_port, buf, bytes_read, NULL, port_write_cb, NULL);

    return TRUE;
}

static void port_opened(SpiceChannel *channel, GParamSpec *pspec,
                        spice_connection *conn)
{
    SpicePortChannel *port = SPICE_PORT_CHANNEL(channel);
    gchar *name = NULL;
    gboolean opened = FALSE;

    g_object_get(channel,
                 "port-name", &name,
                 "port-opened", &opened,
                 NULL);
    
    g_printerr("port %p %s: %s\n", channel, name, opened ? "opened" : "closed");

    if (opened) {
        /* only send a break event and disconnect */
        if (g_strcmp0(name, "org.spice.spicy.break") == 0) {
            spice_port_event(port, SPICE_PORT_EVENT_BREAK);
            spice_channel_flush_async(channel, NULL, port_flushed_cb, conn);
        }

        /* handle the first spicy port and connect it to stdin/out */
        if (g_strcmp0(name, "org.spice.spicy") == 0 && stdin_port == NULL) {
            stdin_port = port;
        }
    } else {
        if (port == stdin_port)
            stdin_port = NULL;
    }

    g_free(name);
}

static void port_data(SpicePortChannel *port,
                      gpointer data, int size, spice_connection *conn)
{
    int r;

    if (port != stdin_port)
        return;

    r = write(fileno(stdout), data, size);
    if (r != size) {
        g_warning("port write failed result %d/%d errno %d", r, size, errno);
    }
}

static void channel_new(SpiceSession *s, SpiceChannel *channel, gpointer data)
{
    spice_connection *conn = data;
    int id;

    g_object_get(channel, "channel-id", &id, NULL);
    conn->channels++;
    SPICE_DEBUG("new channel (#%d)", id);

    if (SPICE_IS_MAIN_CHANNEL(channel)) {
        SPICE_DEBUG("new main channel");
        conn->main = SPICE_MAIN_CHANNEL(channel);
printf("channel 123:\n");
        current_channel = channel;
        g_signal_connect(channel, "channel-event",
                         G_CALLBACK(main_channel_event), conn);
        g_signal_connect(channel, "main-mouse-update",
                         G_CALLBACK(main_mouse_update), conn);
        g_signal_connect(channel, "main-agent-update",
                         G_CALLBACK(main_agent_update), conn);
        main_mouse_update(channel, conn);
        main_agent_update(channel, conn);
    }

    if (SPICE_IS_DISPLAY_CHANNEL(channel)) {  
        if (id >= SPICE_N_ELEMENTS(conn->wins))
            return;
        if (conn->wins[id] != NULL)
            return;
        SPICE_DEBUG("new display channel (#%d)", id);
        
//g_print("display: 123\n");
        g_signal_connect(channel, "notify::monitors",
                         G_CALLBACK(display_monitors), conn);
        spice_channel_connect(channel);
    }

    if (SPICE_IS_INPUTS_CHANNEL(channel)) {
        SPICE_DEBUG("new inputs channel");
        g_signal_connect(channel, "inputs-modifiers",
                         G_CALLBACK(inputs_modifiers), conn);
    }

    if (SPICE_IS_PLAYBACK_CHANNEL(channel)) {
        SPICE_DEBUG("new audio channel");
        conn->audio = spice_audio_get(s, NULL);
    }

    if (SPICE_IS_USBREDIR_CHANNEL(channel)) {
        update_auto_usbredir_sensitive(conn);
    }

    if (SPICE_IS_PORT_CHANNEL(channel)) {
        g_signal_connect(channel, "notify::port-opened",
                         G_CALLBACK(port_opened), conn);
        g_signal_connect(channel, "port-data",
                         G_CALLBACK(port_data), conn);
        spice_channel_connect(channel);
    }
}

static void channel_destroy(SpiceSession *s, SpiceChannel *channel, gpointer data)
{
    spice_connection *conn = data;
    int id;

    g_object_get(channel, "channel-id", &id, NULL);
    if (SPICE_IS_MAIN_CHANNEL(channel)) {
        SPICE_DEBUG("zap main channel");
        conn->main = NULL;
    }

    if (SPICE_IS_DISPLAY_CHANNEL(channel)) {
        if (id >= SPICE_N_ELEMENTS(conn->wins))
            return;
        SPICE_DEBUG("zap display channel (#%d)", id);
        /* FIXME destroy widget only */
    }

    if (SPICE_IS_PLAYBACK_CHANNEL(channel)) {
        SPICE_DEBUG("zap audio channel");
    }

    if (SPICE_IS_USBREDIR_CHANNEL(channel)) {
        update_auto_usbredir_sensitive(conn);
    }

    if (SPICE_IS_PORT_CHANNEL(channel)) {
        if (SPICE_PORT_CHANNEL(channel) == stdin_port)
            stdin_port = NULL;
    }

    conn->channels--;
    if (conn->channels > 0) {
        return;
    }

    connection_destroy(conn);
}

static void migration_state(GObject *session,
                            GParamSpec *pspec, gpointer data)
{
    SpiceSessionMigration mig;

    g_object_get(session, "migration-state", &mig, NULL);
    if (mig == SPICE_SESSION_MIGRATION_SWITCHING)
        g_message("migrating session");
}

static spice_connection *connection_new(void)
{
    spice_connection *conn;
    SpiceUsbDeviceManager *manager;

    conn = g_new0(spice_connection, 1);
    conn->session = spice_session_new();
    conn->gtk_session = spice_gtk_session_get(conn->session);
    g_signal_connect(conn->session, "channel-new",
                     G_CALLBACK(channel_new), conn);
    g_signal_connect(conn->session, "channel-destroy",
                     G_CALLBACK(channel_destroy), conn);
    g_signal_connect(conn->session, "notify::migration-state",
                     G_CALLBACK(migration_state), conn);

    manager = spice_usb_device_manager_get(conn->session, NULL);
    if (manager) {
        g_signal_connect(manager, "auto-connect-failed",
                         G_CALLBACK(usb_connect_failed), NULL);
        g_signal_connect(manager, "device-error",
                         G_CALLBACK(usb_connect_failed), NULL);
    }

    connections++;
    SPICE_DEBUG("%s (%d)", __FUNCTION__, connections);
    return conn;
}

static void connection_connect(spice_connection *conn)
{
    conn->disconnecting = false;
    spice_session_connect(conn->session);
}

static void connection_disconnect(spice_connection *conn)
{
    if (conn->disconnecting)
        return;
    
    if(win->conn == conn) 
        win->conn = NULL;
    
    conn->disconnecting = true;
    spice_session_disconnect(conn->session);
}

static void connection_destroy(spice_connection *conn)
{
    g_object_unref(conn->session);
    free(conn);
    
    connections--;
    SPICE_DEBUG("%s (%d)", __FUNCTION__, connections);
    if (connections > 0) {
        
        return;
    }
    win->conn = NULL;
    //g_main_loop_quit(mainloop);
}

/* ------------------------------------------------------------------ */

static GOptionEntry cmd_entries[] = {
    {
        .long_name        = "full-screen",
        .short_name       = 'f',
        .arg              = G_OPTION_ARG_NONE,
        .arg_data         = &fullscreen,
        .description      = N_("Open in full screen mode"),
    },{
        .long_name        = "version",
        .arg              = G_OPTION_ARG_NONE,
        .arg_data         = &version,
        .description      = N_("Display version and quit"),
    },{
        .long_name        = "title",
        .arg              = G_OPTION_ARG_STRING,
        .arg_data         = &spicy_title,
        .description      = N_("Set the window title"),
        .arg_description  = N_("<title>"),
    },{
        /* end of list */
    }
};

static void (* segv_handler) (int) = SIG_DFL;
static void (* abrt_handler) (int) = SIG_DFL;
static void (* fpe_handler)  (int) = SIG_DFL;
static void (* ill_handler)  (int) = SIG_DFL;
#ifndef G_OS_WIN32
static void (* bus_handler)  (int) = SIG_DFL;
#endif

static void
signal_handler(int signum)
{
    static gint recursion = FALSE;

    /*
     * reset all signal handlers: any further crashes should just be allowed
     * to crash normally.
     * */
    signal(SIGSEGV, segv_handler);
    signal(SIGABRT, abrt_handler);
    signal(SIGFPE,  fpe_handler);
    signal(SIGILL,  ill_handler);
#ifndef G_OS_WIN32
    signal(SIGBUS,  bus_handler);
#endif

    /* Stop bizarre loops */
    if (recursion)
        abort ();

    recursion = TRUE;

    g_main_loop_quit(mainloop);
}

static void usb_connect_failed(GObject               *object,
                               SpiceUsbDevice        *device,
                               GError                *error,
                               gpointer               data)
{
    GtkWidget *dialog;

    if (error->domain == G_IO_ERROR && error->code == G_IO_ERROR_CANCELLED)
        return;

    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                                    GTK_BUTTONS_CLOSE,
                                    "USB redirection error");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                             "%s", error->message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void setup_terminal(gboolean reset)
{
    int stdinfd = fileno(stdin);

    if (!isatty(stdinfd))
        return;

#ifdef HAVE_TERMIOS_H
    static struct termios saved_tios;
    struct termios tios;

    if (reset)
        tios = saved_tios;
    else {
        tcgetattr(stdinfd, &tios);
        saved_tios = tios;
        tios.c_lflag &= ~(ICANON | ECHO);
    }

    tcsetattr(stdinfd, TCSANOW, &tios);
#endif
}

static void watch_stdin(void)
{
    int stdinfd = fileno(stdin);
    GIOChannel *gin;

    setup_terminal(false);
    gin = g_io_channel_unix_new(stdinfd);
    g_io_channel_set_flags(gin, G_IO_FLAG_NONBLOCK, NULL);
    g_io_add_watch(gin, G_IO_IN|G_IO_ERR|G_IO_HUP|G_IO_NVAL, input_cb, NULL);
}

int main(int argc, char *argv[])
{   
    GError *error = NULL;
    GOptionContext *context;
    gchar *conf_file, *conf;
    
    curl_global_init(CURL_GLOBAL_ALL);    //curl
    
#if !GLIB_CHECK_VERSION(2,31,18)
    g_thread_init(NULL);
#endif
    bindtextdomain(GETTEXT_PACKAGE, SPICE_GTK_LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    segv_handler = signal(SIGSEGV, signal_handler);
    abrt_handler = signal(SIGABRT, signal_handler);
    fpe_handler  = signal(SIGFPE,  signal_handler);
    ill_handler  = signal(SIGILL,  signal_handler);
#ifndef G_OS_WIN32
    signal(SIGHUP, signal_handler);
    bus_handler  = signal(SIGBUS,  signal_handler);
#endif

    keyfile = g_key_file_new();

    int mode = S_IRWXU;

    conf_file = g_build_filename(g_get_user_config_dir(), "spicy", NULL);
    if (g_mkdir_with_parents(conf_file, mode) == -1)
        SPICE_DEBUG("failed to create config directory");
    g_free(conf_file);

    //the file in ~/.config/spicy/settings
    conf_file = g_build_filename(g_get_user_config_dir(), "spicy", "settings", NULL);
    if (!g_key_file_load_from_file(keyfile, conf_file,
                                   G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS, &error)) {
        SPICE_DEBUG("Couldn't load configuration: %s", error->message);
        g_clear_error(&error);
    }

    /* parse opts */
    gtk_init(&argc, &argv);

    context = g_option_context_new(_("- spice client test application"));
    g_option_context_set_summary(context, _("Gtk+ test client to connect to Spice servers."));
    g_option_context_set_description(context, _("Report bugs to " PACKAGE_BUGREPORT "."));
    g_option_context_add_group(context, spice_get_option_group());
    g_option_context_set_main_group(context, spice_cmdline_get_option_group());
    g_option_context_add_main_entries(context, cmd_entries, NULL);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_print(_("option parsing failed: %s\n"), error->message);
        exit(1);
    }
    g_option_context_free(context);

    if (version) {
        g_print("spicy " PACKAGE_VERSION "\n");
        exit(0);
    }

#if !GLIB_CHECK_VERSION(2,36,0)
    g_type_init();
#endif

    
    watch_stdin();

    mainloop = g_main_loop_new(NULL, false);
    win = create_spice_window();
    g_main_loop_run(mainloop);

    if(IS_LOGIN)
        delete_cb(NULL, NULL, win);
    g_main_loop_unref(mainloop);

    curl_global_cleanup();   
    
    if ((conf = g_key_file_to_data(keyfile, NULL, &error)) == NULL ||
        !g_file_set_contents(conf_file, conf, -1, &error)) {
        SPICE_DEBUG("Couldn't save configuration: %s", error->message);
        g_error_free(error);
        error = NULL;
    }
   
    g_free(conf_file);
    g_free(conf);
    g_key_file_free(keyfile);

    g_free(spicy_title);

    setup_terminal(true);
 
    return 0;
}

static void vmbar_switch_page(GtkNotebook *notebook, GtkWidget *page, 
                                           guint page_num, gpointer data)
{
    conn_info *ci;                      //lfx2
    char *vmName;
    int status, index=page_num;

    if(index < 2)
    {
        gtk_widget_set_size_request(win->toplevel, 1100, 820);
        gtk_window_resize(GTK_WINDOW(win->toplevel), 1100, 820);
        gtk_widget_queue_resize_no_redraw(win->toplevel);
        update_toolbar_status(-1);
        return ;              
    }
    
    vmName = get_vmbar_vmname(index);
    status = get_vm_status(vmName);
    ci = get_conn_from_list_by_vmname(vmName);

    g_print("vm changed = %s %d %d\n", vmName, index, status); 

    if(NULL == ci)
    {
        gtk_widget_set_size_request(win->toplevel, 1100, 820);
        gtk_window_resize(GTK_WINDOW(win->toplevel), 1100, 820);
        gtk_widget_queue_resize_no_redraw(win->toplevel);
        update_toolbar_status(status);
        return ;
    }
    if(win->conn == ci->conn)
    {
        SpiceDisplay *display = SPICE_DISPLAY(ci->spice);
        SpiceDisplayPrivate *d = display->priv;
        gtk_widget_set_size_request(win->toplevel, d->area.width+164, d->area.height+99);
        gtk_window_resize(GTK_WINDOW(win->toplevel), d->area.width+164, d->area.height+99);
        gtk_widget_queue_resize_no_redraw(win->toplevel);
        update_toolbar_status(status);
        return ;
    }
    if(win->conn != NULL)
    {
        win->conn->wins[win->id * CHANNELID_MAX + win->monitor_id] = NULL;
        g_signal_handlers_block_by_func(win->spice, G_CALLBACK(configure_event_cb), win);
        g_signal_handlers_block_by_func(win->spice, G_CALLBACK(mouse_grab_cb), win);
        g_signal_handlers_block_by_func(win->spice, G_CALLBACK(keyboard_grab_cb), win);
        g_signal_handlers_block_by_func(win->spice, G_CALLBACK(grab_keys_pressed_cb), win);
    }

    SpiceDisplay *display1 = SPICE_DISPLAY(ci->spice);
    SpiceDisplayPrivate *d1 = display1->priv;
    printf("w= %d   h= %d\n\n", d1->area.width, d1->area.height);
    
    win->conn = ci->conn;
    win->display_channel = ci->display_channel;
    win->id = ci->id;
    win->monitor_id = ci->monitor_id;
    win->spice = ci->spice;
    win->conn->wins[win->id * CHANNELID_MAX + win->monitor_id] = win;

    gtk_widget_set_size_request(win->toplevel, d1->area.width+164, d1->area.height+99);
    gtk_window_resize(GTK_WINDOW(win->toplevel), d1->area.width+164, d1->area.height+99);
    gtk_widget_queue_resize_no_redraw(win->toplevel);

    g_signal_handlers_unblock_by_func(win->spice, G_CALLBACK(configure_event_cb), win);
    g_signal_handlers_unblock_by_func(win->spice, G_CALLBACK(mouse_grab_cb), win);
    g_signal_handlers_unblock_by_func(win->spice, G_CALLBACK(keyboard_grab_cb), win);
    g_signal_handlers_unblock_by_func(win->spice, G_CALLBACK(grab_keys_pressed_cb), win);

    update_toolbar_status(status);
    return ;
}
static void on_vmlist_activated(gpointer data)
{
    GtkTreePath *path;
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreeSelection *select;
    
    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(win->vmlist));
    gtk_tree_selection_get_selected(select, &model, &iter);
    path = gtk_tree_model_get_path(model, &iter);
    
    if(gtk_tree_model_iter_has_child(model, &iter))
    {
        if(gtk_tree_view_expand_row(GTK_TREE_VIEW(win->vmlist), path, FALSE))
            ;
        else
            gtk_tree_view_collapse_row(GTK_TREE_VIEW(win->vmlist), path);
    }
    gtk_tree_path_free(path);
}

static SpiceWindow *create_spice_window(void)         //lfx1
{
    char title[32];
    GtkWidget *paned_window, *vbox, *vmlist_vbox, *frame, *scrlled_window;
    GdkPixbuf *image;
    GError *err = NULL;
    
    win = g_object_new(SPICE_TYPE_WINDOW, NULL);

    /* toplevel */
    win->toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    //gtk_widget_set_size_request(win->toplevel, 1100, 820);
    gtk_window_set_default_size(GTK_WINDOW(win->toplevel), 1100, 820);
    
    gtk_window_set_position(GTK_WINDOW(win->toplevel), GTK_WIN_POS_CENTER);
    image = gdk_pixbuf_new_from_file("/etc/linx/ppms/png/home.png", NULL);
    gtk_window_set_icon(GTK_WINDOW(win->toplevel), image);
    if (spicy_title == NULL) {
        snprintf(title, sizeof(title), _("凝思虚拟化连接终端"));
    } else {
        snprintf(title, sizeof(title), "%s", spicy_title);
    }
    gtk_window_set_title(GTK_WINDOW(win->toplevel), title);
    g_signal_connect(G_OBJECT(win->toplevel), "window-state-event",
                     G_CALLBACK(window_state_cb), win);
    g_signal_connect(G_OBJECT(win->toplevel), "delete-event",
                     G_CALLBACK(delete_cb), win);

    /* menu + toolbar */
    win->ui = gtk_ui_manager_new();
    win->ag = gtk_action_group_new("MenuActions");
    gtk_action_group_add_actions(win->ag, entries, G_N_ELEMENTS(entries), win);
    gtk_action_group_add_toggle_actions(win->ag, tentries,
                                        G_N_ELEMENTS(tentries), win);
    gtk_ui_manager_insert_action_group(win->ui, win->ag, 0);
    gtk_window_add_accel_group(GTK_WINDOW(win->toplevel),
                               gtk_ui_manager_get_accel_group(win->ui));
    
    err = NULL;
    if (!gtk_ui_manager_add_ui_from_string(win->ui, ui_xml, -1, &err)) {
        g_warning("building menus failed: %s", err->message);
        g_print("building menus failed: %s", err->message);
        g_error_free(err);
        exit(1);
    }
    //win->menubar = gtk_ui_manager_get_widget(win->ui, "/MainMenu");
   // win->toolbar = gtk_ui_manager_get_widget(win->ui, "/ToolBar");
    win->toolbar = init_toolbar();

    /* recent menu */
    win->ritem  = gtk_ui_manager_get_widget
        (win->ui, "/MainMenu/FileMenu/FileRecentMenu");
    
#ifndef G_OS_WIN32
    GtkRecentFilter  *rfilter;

    win->rmenu = gtk_recent_chooser_menu_new();
    gtk_recent_chooser_set_show_icons(GTK_RECENT_CHOOSER(win->rmenu), FALSE);
    rfilter = gtk_recent_filter_new();
    gtk_recent_filter_add_mime_type(rfilter, "application/x-spice");
    gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(win->rmenu), rfilter);
    gtk_recent_chooser_set_local_only(GTK_RECENT_CHOOSER(win->rmenu), FALSE);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(win->ritem), win->rmenu);
    g_signal_connect(win->rmenu, "item-activated",
                     G_CALLBACK(recent_item_activated_cb), win);
#endif

    /* paned_window  */
#if GTK_CHECK_VERSION(3,0,0)
    //paned_window = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    paned_window = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    //paned_window = gtk_hpaned_new();
    paned_window = gtk_hbox_new(FALSE, 0);
    
#endif
    
    /* vmbar  */
    win->vmbar = init_vmbar();
    g_signal_connect_after(win->vmbar, "switch-page", G_CALLBACK(vmbar_switch_page), win);

    /* vm list  */
    win->vmlist = init_tree();
    scrlled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrlled_window, 150, 780);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrlled_window), 
                                        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrlled_window), win->vmlist);
    
    g_signal_connect(win->vmlist, "row-activated", G_CALLBACK(on_vmlist_activated), NULL);
    
    /* status line */
#if GTK_CHECK_VERSION(3,0,0)
    win->statusbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    win->statusbar = gtk_hbox_new(FALSE, 0);
#endif
    
    win->status = gtk_label_new("status line");
    gtk_misc_set_alignment(GTK_MISC(win->status), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(win->status), 3, 1);
    
    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(win->statusbar), frame, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(frame), win->status);
    
    /* Make a vbox and put stuff in */
#if GTK_CHECK_VERSION(3,0,0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    vmlist_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    vbox = gtk_vbox_new(FALSE, 0);
    vmlist_vbox = gtk_vbox_new(FALSE, 0);
#endif
    
    gtk_container_set_border_width(GTK_CONTAINER(vmlist_vbox), 5);
    gtk_container_add(GTK_CONTAINER(win->toplevel), vbox);
    gtk_box_pack_start(GTK_BOX(vbox), win->toolbar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), paned_window, TRUE, TRUE, 0);
    //gtk_paned_pack1(GTK_PANED(paned_window), vmlist_vbox, FALSE, FALSE);
    //gtk_paned_pack2(GTK_PANED(paned_window), win->vmbar, TRUE, FALSE);
    gtk_box_pack_start(GTK_BOX(paned_window), vmlist_vbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(paned_window), win->vmbar, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vmlist_vbox), gtk_label_new("虚拟机选择列表"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vmlist_vbox), scrlled_window, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(vbox), win->statusbar, FALSE, FALSE, 0);

    /* show window */
    if (fullscreen)
        gtk_window_fullscreen(GTK_WINDOW(win->toplevel));
    gtk_widget_show_all(win->toplevel);
    
    return win;
}

static SpiceWindow* update_spice_window(conn_info *data)
{
    conn_info *ci = data;
    GtkAction *toggle;
    gboolean state;
    GtkWidget *frame;
    SpiceGrabSequence *seq;
    int i;
    
    win->id = ci->id;
    win->monitor_id = ci->monitor_id;
    win->conn = ci->conn;
    win->display_channel = ci->display_channel;

    /* spice display */
	win->spice = ci->spice;
    g_signal_connect(win->spice, "configure-event", G_CALLBACK(configure_event_cb), win);
    seq = spice_grab_sequence_new_from_string("Control_L+Alt_L");
    spice_display_set_grab_keys(SPICE_DISPLAY(win->spice), seq);
    spice_grab_sequence_free(seq);

    g_signal_connect(G_OBJECT(win->spice), "mouse-grab",
                     G_CALLBACK(mouse_grab_cb), win);
    g_signal_connect(G_OBJECT(win->spice), "keyboard-grab",
                     G_CALLBACK(keyboard_grab_cb), win);
    g_signal_connect(G_OBJECT(win->spice), "grab-keys-pressed",
                     G_CALLBACK(grab_keys_pressed_cb), win);
    gtk_widget_show(win->spice);
    
    int index = get_vmbar_index(ci->vmName);
    if(index < 0)
    {
        gtk_notebook_insert_page(GTK_NOTEBOOK(win->vmbar), ci->spice, linx_noetbook_tab_label(ci->vmName), 2);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(win->vmbar), 2);
        
    } else {

        gtk_notebook_insert_page(GTK_NOTEBOOK(win->vmbar), ci->spice, linx_noetbook_tab_label(ci->vmName), index);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(win->vmbar), index);
        gtk_notebook_remove_page(GTK_NOTEBOOK(win->vmbar), index+1);
        gtk_widget_queue_draw(win->vmbar);
    }
    update_status_window(win);

    if(win->st[0] == NULL)
    {    
        for (i = 0; i < STATE_MAX; i++) {
            win->st[i] = gtk_label_new(_("?"));
            gtk_label_set_width_chars(GTK_LABEL(win->st[i]), 5);
            frame = gtk_frame_new(NULL);
            gtk_box_pack_end(GTK_BOX(win->statusbar), frame, FALSE, FALSE, 0);
            gtk_container_add(GTK_CONTAINER(frame), win->st[i]);
        }
    }
    gtk_widget_show_all(win->statusbar);
    
    restore_configuration(win);

    /* init toggle actions */
    for (i = 0; i < G_N_ELEMENTS(spice_display_properties); i++) {
        toggle = gtk_action_group_get_action(win->ag,
                                             spice_display_properties[i]);
        g_object_get(win->spice, spice_display_properties[i], &state, NULL);
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(toggle), state);
    }

    for (i = 0; i < G_N_ELEMENTS(spice_gtk_session_properties); i++) {
        char notify[64];

        toggle = gtk_action_group_get_action(win->ag,
                                             spice_gtk_session_properties[i]);
        g_object_get(win->conn->gtk_session, spice_gtk_session_properties[i],
                     &state, NULL);
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(toggle), state);

        snprintf(notify, sizeof(notify), "notify::%s",
                 spice_gtk_session_properties[i]);
        spice_g_signal_connect_object(win->conn->gtk_session, notify,
                                      G_CALLBACK(menu_cb_conn_bool_prop_changed),
                                      win, 0);
    }
    update_edit_menu_window(win);

    toggle = gtk_action_group_get_action(win->ag, "Toolbar");
    state = gtk_widget_get_visible(win->toolbar);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(toggle), state);

    toggle = gtk_action_group_get_action(win->ag, "Statusbar");
    state = gtk_widget_get_visible(win->statusbar);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(toggle), state);

#ifdef USE_SMARTCARD
    gboolean smartcard;

    enable_smartcard_actions(win, NULL, FALSE, FALSE);
    g_object_get(G_OBJECT(conn->session),
                 "enable-smartcard", &smartcard,
                 NULL);
    if (smartcard) {
        g_signal_connect(G_OBJECT(spice_smartcard_manager_get()), "reader-added",
                         (GCallback)reader_added_cb, win);
        g_signal_connect(G_OBJECT(spice_smartcard_manager_get()), "reader-removed",
                         (GCallback)reader_removed_cb, win);
        g_signal_connect(G_OBJECT(spice_smartcard_manager_get()), "card-inserted",
                         (GCallback)card_inserted_cb, win);
        g_signal_connect(G_OBJECT(spice_smartcard_manager_get()), "card-removed",
                         (GCallback)card_removed_cb, win);
    }
#endif

#ifndef USE_USBREDIR
    GtkAction *usbredir = gtk_action_group_get_action(win->ag, "auto-usbredir");
    gtk_action_set_visible(usbredir, FALSE);
#endif
    gtk_widget_grab_focus(win->spice);

    return win;
}