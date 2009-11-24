# -*- coding:utf-8 -*-

import pygtk
import gtk
import os

class main_GUI:
    
    def __init__(self):
        
        self.default_cmd='/usr/bin/scrot'
        
        self.window = gtk.Window()
        self.window.set_default_size(270, 100)
        
        self.window.set_title("Scrot 截图")
        self.window.set_border_width(3)
        #大小不可改变
        self.window.set_resizable(False)
        #位置居中
        self.window.set_position(gtk.WIN_POS_MOUSE)
    
        self.vbox_main = gtk.VBox()
        
        self.window.add(self.vbox_main)

        #执行的截图命令
        self.cmd='/usr/bin/scrot'

        #设置文件名和路径
        self.name_label = gtk.Label("文件名：")
        self.path_label = gtk.Label("位  置：")
        
        self.name_add = gtk.Entry()
        #self.name_add.set_text('screenscrot')#设置默认名称
        self.path_add = gtk.Entry()
        
        self.path_add.set_text('/home/hcy/');#设置默认路径
        
        self.hbox_name = gtk.HBox()
        self.hbox_name.set_border_width(1)
        self.hbox_name.pack_start(self.name_label, False, True)
        self.hbox_name.pack_start(self.name_add, True, True)
        
        self.hbox_path = gtk.HBox()
        self.hbox_path.set_border_width(1)
        self.hbox_path.pack_start(self.path_label, False, False)
        self.hbox_path.pack_start(self.path_add, True, True)
        
        #加入主窗口中
        self.vbox_main.pack_start(self.hbox_name, False, False)
        self.vbox_main.pack_start(self.hbox_path, False, False)    
        
        #设置截图属性
        self.hbox_argv = gtk.HBox()
        self.hbox_argv.set_border_width(2)
        
        self.button = gtk.RadioButton(None, "截取区域")
        self.button.connect("toggled", self.select, "AREA")
        self.hbox_argv.pack_start(self.button, True, True, 0)
        #默认截取区域
        self.button.set_active(True)
        
    
        self.button = gtk.RadioButton(self.button, "截取窗口")
        self.button.connect("toggled", self.select, "FRAME")
        self.hbox_argv.pack_start(self.button, True, True, 0)
       
        self.button = gtk.RadioButton(self.button, "截取屏幕")
        self.button.connect("toggled", self.select, "SCREEN")
        self.hbox_argv.pack_start(self.button, True, True, 0)

        self.vbox_main.pack_start(self.hbox_argv, False, False)
        
        self.cmd_label=gtk.Label()
        self.cmd_label.set_text('命令：'+self.cmd)
        self.cmd_label.set_justify(gtk.JUSTIFY_LEFT)
        
        self.hobx_cmd_label=gtk.HBox()
        self.hobx_cmd_label.pack_start(self.cmd_label, False, False, 1)
        self.vbox_main.pack_start(self.hobx_cmd_label, False, False)
        
        self.cut_button = gtk.Button("获取截图")
        self.cut_button.connect("clicked", self.clickAction, '')
        
        self.hbox_button = gtk.HBox();
        self.hbox_button.pack_start(self.cut_button, True, False, 5)
        self.vbox_main.pack_start(self.hbox_button, False, False)
        
        
        
        #默认截取区域
        self.select(None, "AREA")
        
        self.window.show_all()
        self.window.connect('destroy', lambda q:gtk.main_quit())
        
       
    def select(self,widgte=None,data=None):
        
        if data=='AREA':
            print '截取区域'
            self.cmd=self.default_cmd+' -s -b'
            self.cmd_label.set_text('命令：'+self.cmd)
        elif data=='FRAME':
            print '截取窗口'
            self.cmd=self.default_cmd+' -sb -cd 2'#延时2秒
            self.cmd_label.set_text('命令：'+self.cmd)
        elif data=='SCREEN':
            self.cmd=self.default_cmd
            self.cmd_label.set_text('命令：'+self.cmd)
            print '截取屏幕'
        
        
    def clickAction(self, widgte=None, data=None):
        """
                截图按钮的动作函数
        """
        
        print "获取截图"
        #隐藏主窗口
        self.window.hide_all()    
        
        
        #设置保存的路径和文件名
        self.cmd+=' -e \'mv $f '
        self.cmd+=self.path_add.get_text()
        self.cmd+=self.name_add.get_text()
        self.cmd+='.png'
        self.cmd+='\''
        self.cmd_label.set_text(self.cmd)
        
        print self.cmd
        '/usr/bin/scrot -cd 1 -q 85 -s -b -e \'mv $f ~/screenshots/ss\''
        #执行截图命令
        os.system(self.cmd)
        #subproc = subprocess.Popen([self.cmd], stdin=subprocess.PIPE, shell=True)
        #subproc.stdin.write('data\n')
        
        
        
        
        self.dialog_set=gtk.Dialog("信息",self.window,gtk.DIALOG_MODAL)
        #点击对话框关闭按钮默认回到主窗口
        self.dialog_set.connect('destroy', self.dialog_another_action)
        
        self.dialog_another=gtk.Button("再截一个!!")
        self.dialog_another.connect("clicked",self.dialog_another_action,'another')
        
        self.dialog_exit=gtk.Button("不截了，走喽...")
        self.dialog_exit.connect("clicked",self.dialog_exit_main_action,'exit')
        
        self.dialog_hbox=gtk.HBox()
        self.dialog_hbox.pack_start(self.dialog_another, True, False, 5)
        self.dialog_hbox.pack_start(self.dialog_exit, True, False, 0)
        
        
        self.info='\b截图成功！！\n存储位置：'+self.path_add.get_text()+'\n名称：'
        self.dialog_info_label=gtk.Label(self.info);
        
        self.dialog_set.vbox.pack_start(self.dialog_info_label, False, False, 5)
        self.dialog_set.vbox.pack_start(self.dialog_hbox, False, True, 5)
        self.dialog_set.set_default_size(200,70)
        
        
        
        self.dialog_set.show_all()
        
        
        
    def dialog_another_action(self, widgte=None, data=None):
        """
                对话框按钮的动作函数
        """
        self.dialog_set.destroy()
        self.cmd=self.default_cmd+' -s -b'
        self.cmd_label.set_text('命令：'+self.cmd)
        #显示主窗口
        #位置居中
        self.window.set_position(gtk.WIN_POS_NONE)
        self.window.show_all()
        print data
        
    def dialog_exit_main_action(self, widgte=None, data=None):
        """
                退出程序
        """
        print data
        self.dialog_set.destroy()
        gtk.main_quit()
        
        
    def main(self):
        gtk.main()
    
    
main_GUI().main()