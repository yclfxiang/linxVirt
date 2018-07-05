# coding=utf-8

from src.pages.vm import VmPage


class VmCase(VmPage):

    # 新建虚拟机
    def new_vm(self, name, description, memsize, maxszie, cpu):
        self.send_input_nameN(name)
        self.sleep(1)
        self.send_input_descriptionN(description)
        self.sleep(1)
        self.send_input_memsizeN(memsize)
        self.sleep(1)
        self.send_input_maxsizeN(maxszie)
        self.sleep(1)
        self.send_input_cpuN(cpu)

    # 新建虚拟机附加磁盘
    def new_vm_attach_disk(self):
        self.send_click_submitAT()

    # 新建虚拟机新建磁盘
    def new_vm_new_disk(self, name, description, size):
        self.send_input_nameVDA(name)
        self.sleep(1)
        self.send_input_descriptionVDA(description)
        self.sleep(1)
        self.send_input_sizeVDA(size)
        self.sleep(1)
        self.send_click_submitVDA()

    # 使用模板新建虚拟机
    def new_vm_temp(self, temp, number, name, description, memsize, maxsize, cpu):
        self.send_select_tempNT(type)
        self.sleep(1)
        self.send_input_numberNT(number)
        self.sleep(1)
        self.send_input_nameNT(name)
        self.sleep(1)
        self.send_input_descriptionNT(description)
        self.sleep(1)
        self.send_input_memsizeNT(memsize)
        self.sleep(1)
        self.send_input_maxsizeNT(maxsize)
        self.sleep(1)
        self.send_input_cpuNT(cpu)
        self.sleep(1)
        self.send_click_submitNT()

    # 导入虚拟机
    def import_vm(self):
        self.send_click_submitIP()

    # 编辑虚拟机
    def edit_vm(self, name, description, memsize, maxsize, cpu, kcpu, tcpu):
        self.send_input_nameE(name)
        self.sleep(1)
        self.send_input_descriptionE(description)
        self.sleep(1)
        self.send_input_memsizeE(memsize)
        self.sleep(1)
        self.send_input_maxsizeE(maxsize)
        self.sleep(1)
        self.send_input_cpuE(cpu)
        self.sleep(1)
        self.send_input_kcpuE(kcpu)
        self.sleep(1)
        self.send_input_tcpuE(tcpu)
        self.sleep(1)
        self.send_click_submitE()

    # 删除虚拟机
    def delete_vm(self, deldisk, cleardisk):
        if "1" == deldisk:
            self.send_click_removediskD()
            self.sleep(1)
            if "1" == cleardisk:
                self.send_click_cleardiskD()
                self.sleep(1)
        else:
            self.send_click_unremovediskD()
        self.sleep(1)
        self.send_click_submitD()

    # 运行一次虚拟机
    def runonce_vm(self, boot, iso):
        self.send_select_cdRN(iso)
        self.sleep(1)
        if "PXE" == boot:
            self.send_select_bootRN("网络(PXE)")
            self.sleep(1)
            self.send_click_upRN()
        if "CD" == boot:
            self.send_select_bootRN("CD-ROM")
            self.sleep(1)
            self.send_click_upRN()
            self.sleep(1)
            self.send_click_upRN()
        self.sleep(1)
        self.send_click_submitRN()

    # 启动虚拟机
    def start_vm(self):
        self.send_click_start()

    # 暂停虚拟机
    def pause_vm(self):
        self.send_click_pause()

    # 重启虚拟机
    def restart_vm(self):
        self.send_click_restart()

    # 关闭虚拟机
    def stop_vm(self):
        self.send_click_stop()

    # 迁移虚拟机
    def migrate_vm(self, bandwidth):
        self.send_input_bandwidthM(bandwidth)
        self.sleep(1)
        self.send_click_submitM()

    # 导出虚拟机
    def export_vm(self):
        self.send_click_submitEP()

    # 虚拟机转化为模板
    def turn_temp(self, name, description):
        self.send_input_nameT(name)
        self.sleep(1)
        self.send_input_descriptionT(description)
        self.sleep(1)
        self.send_click_submitT()

    # 切换CD
    def change_cd(self, iso):
        self.send_select_cdCD(iso)
        self.sleep(1)
        self.send_click_submitCD()

    # 创建快照
    def new_snapshot(self, name, description):
        self.send_input_nameS(name)
        self.sleep(1)
        self.send_input_descriptionS(description)
        self.sleep(1)
        self.send_click_submitS()

    # 恢复快照
    def delete_snapshot(self):
        self.send_click_submitRS()

    # 删除快照
    def restore_snapshot(self):
        self.send_click_submitDS()