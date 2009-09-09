# -*- coding:utf-8 -*-

import pygtk
import gtk
import os
import send_mail 

class main_GUI:
    
    def __init__(self):
        self.window = gtk.Window()
        self.window.set_default_size(500, 300)
        self.window.set_title("Mail Sender")
        self.window.set_border_width(0)

    
        self.vbox_main = gtk.VBox()
        
        self.window.add(self.vbox_main)

        self.main_menu = gtk.Menu()
        self.root_menu = gtk.MenuItem("设置")
        self.root_menu.connect("activate",self.set_config,self)
        self.root_menu.set_submenu(self.main_menu)

        
        self.root_about_menu = gtk.MenuItem("帮助")
        
        self.about_menu = gtk.Menu()
        self.root_about_menu.set_submenu(self.about_menu);

        self.about_menu_sub=gtk.MenuItem("关于")
        self.about_menu.append(self.about_menu_sub)
        
        self.menu_bar = gtk.MenuBar()
        self.menu_bar.add(self.root_menu);
        self.menu_bar.append(self.root_about_menu)
#        self.menu_bar.add(self.root_about_menu)

        self.vbox_main.pack_start(self.menu_bar, False, False,0)

        self.from_label= gtk.Label("发信人：")
        self.to_label = gtk.Label("收信人：")
        self.sub_label = gtk.Label("主  题：")
        self.from_add=gtk.Entry()
        self.to_add=gtk.Entry()
        self.sub = gtk.Entry()
        
        self.hbox_from=gtk.HBox()
        self.hbox_from.set_border_width(5)
        self.hbox_from.pack_start(self.from_label,False,True)
        self.hbox_from.pack_start(self.from_add,True,True)
        
        self.hbox_to=gtk.HBox()
        self.hbox_to.set_border_width(5)
        self.hbox_to.pack_start(self.to_label,False,False)
        self.hbox_to.pack_start(self.to_add,True,True)
        
        self.hbox_sub=gtk.HBox()
        self.hbox_sub.set_border_width(5)
        self.hbox_sub.pack_start(self.sub_label,False,False)
        self.hbox_sub.pack_start(self.sub,True,True)
        
        self.vbox_main.pack_start(self.hbox_from, False, False)
        self.vbox_main.pack_start(self.hbox_to, False, False)    
        self.vbox_main.pack_start(self.hbox_sub, False, False)
        
        self.content_label=gtk.Label("信件内容：");
        self.content_label.set_justify(gtk.JUSTIFY_LEFT)
        self.hbox_con=gtk.HBox()
        self.hbox_con.set_border_width(5)
        self.hbox_con.pack_start(self.content_label,False,False)
        
        self.sw = gtk.ScrolledWindow()
        self.sw.set_policy(gtk.POLICY_NEVER, gtk.POLICY_AUTOMATIC)
        self.sw.set_border_width(5)
        self.textview = gtk.TextView()
        self.textview.set_wrap_mode(gtk.WRAP_WORD)
        
        self.textbuffer = self.textview.get_buffer()
        self.sw.add(self.textview)

        self.vbox_main.pack_start(self.hbox_con, False, False)
        self.vbox_main.pack_start(self.sw, False, False)
        
        self.send_button = gtk.Button("发送!")
        self.send_button.connect("clicked",self.clickAction,'')
        
        self.hbox_button=gtk.HBox();
        self.hbox_button.pack_start(self.send_button,True,False,5)
        self.vbox_main.pack_start(self.hbox_button, False, False)
        
        self.window.show_all()
        self.window.connect('destroy',lambda q:gtk.main_quit())
        
        self.deal_mail=send_mail.send_Mail()
        print self.deal_mail
        self.status=False
        self.info=''
        
    def clickAction(self,widgte,data):
        
        self.deal_mail.set_from(self.from_add.get_text())
        self.deal_mail.set_to_list(self.to_add.get_text())
        self.deal_mail.set_subject(self.sub.get_text())
        self.deal_mail.set_content(self.textview)
        
        self.status,self.info = self.deal_mail.send_mail()
        dialog=gtk.Dialog("信息",self.window,gtk.DIALOG_MODAL)
        label=gtk.Label(str(self.info))
        but=gtk.Button("确定")
        but.connect("clicked",lambda widgte,data: dialog.destroy(),'')
        dialog.vbox.pack_start(label, True, True, 0)
        dialog.vbox.pack_start(but, True, True, 0)
        dialog.set_default_size(200,100)
        dialog.show_all()
        
        
    def set_config(self,widgte,data):
        
        dialog_set=gtk.Dialog("设置",self.window,gtk.DIALOG_MODAL)
        but_set=gtk.Button("确定")
        but_set.connect("clicked",lambda widgte,data: dialog_set.destroy(),'')
        dialog_set.vbox.pack_start(but_set, True, True, 0)
        dialog_set.set_default_size(200,100)
        dialog_set.show_all()
        
        
        
        
    def main(self):
        gtk.main()
    
    
main_GUI().main()