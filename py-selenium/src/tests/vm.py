# coding=utf-8

from src.usecases.vm import VmCase
from selenium.webdriver.common.action_chains import ActionChains


newVmAttachDisk = {"name":"nod", "description":"12", "memsize":"500", "maxszie":"2000", "cpu":"1", "disk":"linx"}
newVmNewDisk = {"vmname":"www", "vmdescription":"12", "memsize":"500",
                "maxszie":"2000", "cpu":"1", "diskname":"www-disk", "diskdescription":"12", "size":"30"}
newVmTemp = {"temp":"", "number":"2", "name":"mmm", "description":"12", "memsize":"500", "maxszie":"2000", "cpu":"1"}
importVm = {"vm":"mmm-1"}
editVm = {"vm":"", "name":"", "mac":"", "description":"", "memsize":"", "maxsize":"", "cpu":"", "kcpu":"", "tcpu":""}
deleteVm = {"vm":"www", "deldisk":"0", "cleardisk":"0"}
runonceVm = {"vm":"test", "boot":"PXE", "iso":""}
startVm = {"vm":"test"}
pauseVm = {"vm":"test"}
restartVm = {"vm":"test"}
stopVm = {"vm":"test"}
migrateVm = {"vm":"test", "bandwidth":"500"}
exportVm = {"vm":"mmm-1"}
turnVmTemp = {"vm":"", "name":"", "description":""}
changeCd = {"vm":"", "iso":""}
newSnap = {"vm":"test", "name":"snap1", "description":"123"}
restoreSnap = {"vm":"test", "snap":"1"}
deleteSnap = {"vm":"node-1", "snap":"1"}


class TestVm(VmCase):

    # 测试新建虚拟机功能(附加磁盘)
    def test_new_vm_attach_disk(self, name, description, memsize, maxszie, cpu, disk):
        self.send_click_new0()
        self.sleep(1)
        self.send_click_new()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.new_vm(name, description, memsize, maxszie, cpu)
        self.sleep(1)
        self.send_click_attachN()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        if 0 == self.select_table_row(self.at_table, disk):
            self.logger.info("********** 要附加到虚拟机的磁盘不存在，测试结束! *********")
            self.send_click_closeAT()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
            self.sleep(1)
            self.send_click_closeN()
            self.sleep(1)
            self.driver.switch_to.frame("vm")
            return
        self.sleep(1)
        self.new_vm_attach_disk()
        self.sleep(1)
        self.send_click_submitR()
        self.sleep(1)
        self.driver.switch_to.parent_frame()
        self.sleep(1)
        self.send_click_submitN()
        self.sleep(1)
        result = self.get_element_text(self.n_result)
        if "成功" in result:
            self.logger.info("********** (附加磁盘)新建虚拟机功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("vm")
        else:
            self.logger.info("(附加磁盘)新建虚拟机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeN()
            self.sleep(1)
            self.driver.switch_to.frame("vm")

    # 测试新建虚拟机功能(新建磁盘)
    def test_new_vm_new_disk(self, vmname, vmdescription, memsize, maxszie, cpu, diskname, diskdescription, size):
        self.send_click_new0()
        self.sleep(1)
        self.send_click_new()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.new_vm(vmname, vmdescription, memsize, maxszie, cpu)
        self.sleep(1)
        self.send_click_newdiskN()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.new_vm_new_disk(diskname, diskdescription, size)
        self.sleep(1)
        self.send_click_submitR()
        self.sleep(1)
        self.driver.switch_to.parent_frame()
        self.sleep(1)
        self.send_click_submitN()
        self.sleep(1)
        result = self.get_element_text(self.n_result)
        if "成功" in result:
            self.logger.info("********** (新建磁盘)新建虚拟机功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("vm")
        else:
            self.logger.info("(新建磁盘)新建虚拟机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeN()
            self.sleep(1)
            self.driver.switch_to.frame("vm")

    # 测试使用模板新建虚拟机功能
    def test_new_vm_with_temp(self, temp, number, name, description, memsize, maxszie, cpu):
        self.send_click_new0()
        self.sleep(1)
        self.send_click_new_with_temp()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.new_vm_temp(temp, number, name, description, memsize, maxszie, cpu)
        self.sleep(1)
        result = self.get_element_text(self.nt_result)
        if "成功" in result:
            self.logger.info("********** 批量新建虚拟机功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("vm")
        else:
            self.logger.info("批量新建虚拟机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeNT()
            self.sleep(1)
            self.driver.switch_to.frame("vm")

    # 测试导入虚拟机功能
    def test_import_vm(self, vm):
        self.send_click_import()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        if 0 == self.select_table_row(self.ip_table, vm):
            self.logger.info("********** 要导入的虚拟机不在备份域中，测试结束! *********")
            self.send_click_closeIP()
            self.sleep(1)
            self.driver.switch_to.frame("vm")
            return
        self.sleep(1)
        self.import_vm()
        self.sleep(1)
        result = self.get_element_text(self.ip_result)
        if "成功" in result:
            self.logger.info("********** 导入虚拟机功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("vm")
        else:
            self.logger.info("导入虚拟机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeIP()
            self.sleep(1)
            self.driver.switch_to.frame("vm")

    # 测试编辑虚拟机功能
    def test_edit_vm(self, vm, name, description, memsize, maxsize, cpu, kcpu, tcpu):
        if 0 == self.select_vm(vm):
            self.logger.info("********** 要编辑的虚拟机不存在，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_edit()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_selby_tag_name("iframe"))
        self.sleep(1)
        self.edit_vm(name, description, memsize, maxsize, cpu, kcpu, tcpu)
        self.sleep(1)
        result = self.get_element_text(self.e_result)
        if "成功" in result:
            self.logger.info("********** 编辑虚拟机功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("vm")
        else:
            self.logger.info("编辑虚拟机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeE()
            self.sleep(1)
            self.driver.switch_to.frame("vm")

    # 测试删除虚拟机
    def test_delete_vm(self, vm, deldisk, cleardisk):
        if 0 == self.select_vm(vm):
            self.logger.info("********** 要删除的虚拟机不存在，测试结束! *********")
            return
        self.sleep(1)
        status = self.get_vm_status(vm)
        if "离线" not in status:
            self.logger.info("********** 要删除的虚拟机不是离线状态，不能被删除，测试结束! *********")
            return
        self.send_click_delete()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.delete_vm(deldisk, cleardisk)
        self.sleep(1)
        result = self.get_element_text(self.d_result)
        if "成功" in result:
            self.logger.info("********** 删除虚拟机功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("vm")
        else:
            self.logger.info("删除虚拟机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeD()
            self.sleep(1)
            self.driver.switch_to.frame("vm")

    # 测试运行一次虚拟机功能
    def test_runonce_vm(self, vm, boot, iso):
        if 0 == self.select_vm(vm):
            self.logger.info("********** 要启动(运行一次)的虚拟机不存在，测试结束! *********")
            return
        self.sleep(1)
        status = self.get_vm_status(vm)
        if "离线" not in status:
            self.logger.info("********** 要启动(运行一次)的虚拟机不是离线状态，测试结束! *********")
            return
        self.send_click_runonce()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.runonce_vm(boot, iso)
        self.sleep(1)
        result = self.get_element_text(self.rn_result)
        if "成功" in result:
            self.logger.info("********** 运行一次虚拟机功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("vm")
        else:
            self.logger.info("运行一次虚拟机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeRN()
            self.sleep(1)
            self.driver.switch_to.frame("vm")

    # 测试启动虚拟机功能
    def test_start_vm(self, vm):
        self.select_vm(vm)
        self.sleep(1)
        self.start_vm()

    # 测试暂停虚拟机功能
    def test_pause_vm(self, vm):
        self.select_vm(vm)
        self.sleep(1)
        self.pause_vm()

    # 测试重启虚拟机功能
    def test_restart_vm(self, vm):
        self.select_vm(vm)
        self.sleep(1)
        self.restart_vm()

    # 测试关闭虚拟机功能
    def test_stop_vm(self, vm):
        self.select_vm(vm)
        self.sleep(1)
        self.stop_vm()

    # 测试迁移虚拟机功能
    def test_migrate_vm(self, vm, bandwidth):
        if 0 == self.select_vm(vm):
            self.logger.info("********** 要迁移的虚拟机不存在，测试结束! *********")
            return
        self.sleep(1)
        status = self.get_vm_status(vm)
        if "在线" not in status:
            self.logger.info("********** 要迁移的虚拟机不是在线状态，测试结束! *********")
            return
        self.send_click_migrate()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.migrate_vm(bandwidth)
        self.sleep(1)
        result = self.get_element_text(self.m_result)
        if "成功" in result:
            self.logger.info("********** 迁移虚拟机功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("vm")
        else:
            self.logger.info("迁移虚拟机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeM()
            self.sleep(1)
            self.driver.switch_to.frame("vm")

    # 测试导出虚拟机功能
    def test_export_vm(self, vm):
        if 0 == self.select_vm(vm):
            self.logger.info("********** 要导出的虚拟机不存在，测试结束! *********")
            return
        self.sleep(1)
        status = self.get_vm_status(vm)
        if "离线" not in status:
            self.logger.info("********** 要导出的虚拟机不是离线状态，测试结束! *********")
            return
        self.send_click_export()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.export_vm()
        self.sleep(1)
        result = self.get_element_text(self.ep_result)
        if "成功" in result:
            self.logger.info("********** 导出虚拟机功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("vm")
        else:
            self.logger.info("导出虚拟机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeEP()
            self.sleep(1)
            self.driver.switch_to.frame("vm")

    # 测试虚拟机转化为模板功能
    def test_turn_temp(self, vm, name, description):
        if 0 == self.select_vm(vm):
            self.logger.info("********** 要转化为模板的虚拟机不存在，测试结束! *********")
            return
        self.sleep(1)
        status = self.get_vm_status(vm)
        if "离线" not in status:
            self.logger.info("********** 要转化为模板的虚拟机不是离线状态，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_totemp()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.turn_temp(name, description)
        self.sleep(1)
        result = self.get_element_text(self.t_result)
        if "成功" in result:
            self.logger.info("********** 虚拟机转化为模板功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("vm")
        else:
            self.logger.info("虚拟机转化为模板功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeEP()
            self.sleep(1)
            self.driver.switch_to.frame("vm")

    # 测试切换CD功能
    def test_change_cd(self, vm, iso):
        if 0 == self.select_vm(vm):
            self.logger.info("********** 要切换CD的虚拟机不存在，测试结束! *********")
            return
        self.sleep(1)
        status = self.get_vm_status(vm)
        if "在线" not in status:
            self.logger.info("********** 要切换CD的虚拟机不是在线状态，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_changcd()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.change_cd(iso)
        self.sleep(1)
        result = self.get_element_text(self.cd_result)
        if "成功" in result:
            self.logger.info("********** 切换CD功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("vm")
        else:
            self.logger.info("切换CD功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeCD()
            self.sleep(1)
            self.driver.switch_to.frame("vm")

    # 测试创建快照功能
    def test_new_snap(self, vm, name, description):
        if 0 == self.select_vm(vm):
            self.logger.info("********** 要创建快照的虚拟机不存在，测试结束! *********")
            return
        self.sleep(1)
        status = self.get_vm_status(vm)
        if "在线" not in status:
            self.logger.info("********** 要创建快照的虚拟机不是在线状态，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_takesnap()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.new_snapshot(name, description)
        self.sleep(1)
        result = self.get_element_text(self.s_result)
        if "成功" in result:
            self.logger.info("********** 创建快照功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("vm")
        else:
            self.logger.info("创建快照功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeS()
            self.sleep(1)
            self.driver.switch_to.frame("vm")

    # 测试恢复快照功能
    def test_restore_snap(self, vm, snap):
        if 0 == self.select_vm(vm):
            self.logger.info("********** 要恢复快照的虚拟机不存在，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_managesnap()
        self.sleep(1)
        base = self.driver.find_element_by_xpath(self.rs_base)
        ac = ActionChains(self.driver)
        ac.move_to_element(base).perform()
        self.sleep(1)
        if "1" == snap:
            ac.move_by_offset(40, 155).click().perform()
        if "2" == snap:
            ac.move_by_offset(130, 155).click().perform()
        if "3" == snap:
            ac.move_by_offset(220, 155).click().perform()
        self.sleep(1)
        self.send_click_resnap()
        self.sleep(1)
        self.restore_snapshot()
        self.sleep(1)
        result = self.get_element_text(self.rs_result)
        if "成功" in result:
            self.logger.info("********** 恢复快照功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
        else:
            self.logger.info("恢复快照功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeRS()

    # 测试删除快照功能
    def test_delete_snap(self, vm, snap):
        if 0 == self.select_vm(vm):
            self.logger.info("********** 要删除快照的虚拟机不存在，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_managesnap()
        self.sleep(1)
        base = self.driver.find_element_by_xpath(self.ds_base)
        ac = ActionChains(self.driver)
        ac.move_to_element(base).perform()
        self.sleep(1)
        if "1" == snap:
            ac.move_by_offset(40, 155).click().perform()
        if "2" == snap:
            ac.move_by_offset(130, 155).click().perform()
        if "3" == snap:
            ac.move_by_offset(220, 155).click().perform()
        self.sleep(1)
        self.send_click_delsnap()
        self.sleep(1)
        self.delete_snapshot()
        self.sleep(1)
        result = self.get_element_text(self.ds_result)
        if "成功" in result:
            self.logger.info("********** 删除快照功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
        else:
            self.logger.info("删除快照功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeDS()