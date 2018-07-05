# coding=utf-8

from src.pages.base import BasePage


class VmPage(BasePage):

    # menu bun 获取前需切换iframe => self.driver.switch_to.frame("vm")
    new0 = "xpath=>/html/body/div/fieldset/blockquote/ul"
    new = "id=>newVM"
    new_with_temp = "id=>templateNewVM"
    load = "id=>importVM"
    edit = "id=>editVM"
    delete = "id=>removeVM"
    runonce = "id=>oneRunVM"
    start = "id=>runVM"
    pause = "id=>pauseVM"
    restart = "id=>rebootVM"
    stop = "id=>shutdownVM"
    migrate = "id=>migrateVM"
    export = "id=>exportVM"
    totemp = "id=>creatTemplate"
    takesnap = "id=>creatSnapshot"
    managesnap  = "id=>snapshotInfobtn"
    resnap = "id=>restoreSnap"
    delsnap = "id=>deleteSnap"
    changecd = "id=>changeCD"

# vm info table
    table = "xpath=>//*[@id='iVMTable']"

# new create vm
    n_name = "name=>vmName"
    n_description = "name=>description"
    n_memsize = "name=>memSize"
    n_maxsize = "name=>maxMemSize"
    n_cpu = "name=>smp"
    n_network= "name=>displayNetwork"
    n_graphics = "name=>graphics"
    n_video = "name=>video"
    n_close = "id=>cancle"
    n_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    n_result = "xpath=>/html/body/div[3]/div[2]"

    # 磁盘添加方式
    n_attach = "id=>attachDisk"
    n_newdisk = "id=>newDisk"
    # new create vm with attach disk     独立的iframe,需却换iframe
    at_table = "xpath=>//*[@id='content-table']/tbody"
    at_close = "id=>cancle"
    at_submit = "xpath=>//*[@id='attachDisk']"
    # new create vm with new create disk   独立的iframe,需却换iframe
    vda_name = "name=>diskAlias"
    vda_description = "name=>description"
    vda_size = "name=>size"
    vda_domain = ""
    vda_type = ""
    vda_close = "id=>cancle"
    vda_submit = "xpath=>/html/body/form/div[2]/div/button[2]"

# new create vm with temp
    nt_temp = "xpath=>/html/body/form/div[1]/div[2]/div/div"
    nt_number = "name=>num"
    nt_name = "name=>name"
    nt_description = "name=>description"
    nt_memsize = "name=>memSize"
    nt_maxsize = "name=>maxMemSize"
    nt_cpu = "name=>smp"
    nt_close = "id=>cancle"
    nt_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    nt_result = "xpath=>/html/body/div[2]/div[2]"

# import vm
    ip_domain = "name=>domainName"
    ip_table = "xpath=>//*[@id='content-table']/tbody"
    ip_close = "id=>cancle"
    ip_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    ip_result = "xpath=>/html/body/div[5]/div[2]"

# edit vm
    e_name = "name=>vmName"
    e_mac = "name=>macAddr"
    e_description = "name=>description"
    e_memsize = "name=>memSize"
    e_maxsize = "name=>maxMemSize"
    e_cpu = "name=>smp"
    e_kcpu = "name=>smpCoresPerSocket"
    e_tcpu = "name=>smpThreadsPerCore"
    e_close = "id=>cancle"
    e_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    e_result = "xpath=>/html/body/div[2]/div[2]"

# delete vm
    d_removedisk = "xpath=>/html/body/form/div/div[2]/div[1]/div"
    d_unremovedisk = "xpath=>/html/body/form/div/div[2]/div[3]/div"
    d_cleardisk = "xpath=>/html/body/form/div/div[2]/div[2]/div"
    d_close = "id=>cancle"
    d_submit = "xpath=>/html/body/form/div/div[3]/div/button[2]"
    d_result = "xpath=>/html/body/div[2]/div[2]"

# runonce vm
    rn_iso = "/html/body/form/div[1]/div[1]/div/div"
    rn_refresh = "xpath=>//*[@id='isolistRefresh']"
    rn_boot = "xpath=>//*[@id='bootOrder']"
    rn_up = "xpath=>//*[@id='moveUp']"
    rn_down = "xpath=>//*[@id='moveDown']"
    rn_close = "id=>cancle"
    rn_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    rn_result = "xpath=>/html/body/div[3]/div[2]"

# migrate vm
    m_host = "xpath=>//*[@id='selectID']"
    m_bandwidth = "name=>bandwidth"
    m_close = "id=>cancle"
    m_submit = "xpath=>/html/body/form/div[2]/button[2]"
    m_result = "xpath=>/html/body/div[3]/div[2]"

# export vm
    ep_close = "id=>cancle"
    ep_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    ep_result = "xpath=>/html/body/div[2]/div[2]"

# turn tepm
    t_name = "name=>name"
    t_description = "name=>description"
    t_close = "id=>cancle"
    t_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    t_result = "xpath=>/html/body/div[3]/div[2]"

# take snap
    s_name = "name=>name"
    s_description = "name=>description"
    s_close = "id=>cancle"
    s_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    s_result = "xpath=>/html/body/div[3]/div[2]"

# restore snap
    rs_base = "//*[@id='basicInfobtn']"
    rs_close = "class_name=>layui-layer-btn0"
    rs_submit = "class_name=>layui-layer-btn1"
    rs_result = "xpath=>/html/body/div[4]/div[2]"

# delete snap
    ds_base = "//*[@id='basicInfobtn']"
    ds_close = "class_name=>layui-layer-btn0"
    ds_submit = "class_name=>layui-layer-btn1"
    ds_result = "xpath=>/html/body/div[4]/div[2]"

# change cd
    cd_iso = "/html/body/form/div[1]/div[2]/div/div"
    cd_iso_select = ""
    cd_refresh = "xpath=>//*[@id='isolistRefresh']"
    cd_close = "id=>cancle"
    cd_submit = "xpath=>/html/body/form/div[2]/div/button[2]"
    cd_result = "xpath=>/html/body/div[3]/div[2]"

# execute result submit
    r_submit = "class_name=>layui-layer-btn0"

# to execute menu
    def send_click_new0(self):
        self.click(self.new0)

    def send_click_new(self):
        self.click(self.new)

    def send_click_new_with_temp(self):
        self.click(self.new_with_temp)

    def send_click_import(self):
        self.click(self.load)

    def send_click_edit(self):
        self.click(self.edit)

    def send_click_delete(self):
        self.click(self.delete)

    def send_click_runonce(self):
        self.click(self.runonce)

    def send_click_start(self):
        self.click(self.start)

    def send_click_pause(self):
        self.click(self.pause)

    def send_click_restart(self):
        self.click(self.restart)

    def send_click_stop(self):
        self.click(self.stop)

    def send_click_migrate(self):
        self.click(self.migrate)

    def send_click_export(self):
        self.click(self.export)

    def send_click_totemp(self):
        self.click(self.totemp)

    def send_click_takesnap(self):
        self.click(self.takesnap)

    def send_click_managesnap(self):
        self.click(self.managesnap)

    def send_click_resnap(self):
        self.click(self.resnap)

    def send_click_delsnap(self):
        self.click(self.delsnap)

    def send_click_changcd(self):
        self.click(self.changecd)

# to new vm
    def send_input_nameN(self, text):
        self.input(self.n_name, text)

    def send_input_descriptionN(self, text):
        self.input(self.n_description, text)

    def send_input_memsizeN(self, text):
        self.input(self.n_memsize, text)

    def send_input_maxsizeN(self, text):
        self.input(self.n_maxsize, text)

    def send_input_cpuN(self, text):
        self.input(self.n_cpu, text)

    def send_click_attachN(self):
        self.click(self.n_attach)

    def send_click_newdiskN(self):
        self.click(self.n_newdisk)

    # pass: select network、spice、video

    def send_click_closeN(self):
        self.click(self.n_close)

    def send_click_submitN(self):
        self.click(self.n_submit)

    # to attach disk
    def send_click_closeAT(self):
        self.click(self.at_close)

    def send_click_submitAT(self):
        self.click(self.at_submit)

    # to new create disk
    def send_input_nameVDA(self, text):
        self.input(self.vda_name, text)

    def send_input_descriptionVDA(self, text):
        self.input(self.vda_description, text)

    def send_input_sizeVDA(self, text):
        self.input(self.vda_size, text)

    def send_click_closeVDA(self):
        self.click(self.vda_close)

    def send_click_submitVDA(self):
        self.click(self.vda_submit)

# to new vm with temp
    def send_select_tempNT(self, temp):
        self.click_select_option(self.nt_temp, temp)

    def send_input_numberNT(self, text):
        self.input(self.nt_number, text)

    def send_input_nameNT(self, text):
        self.input(self.nt_name, text)

    def send_input_descriptionNT(self, text):
        self.input(self.nt_description, text)

    def send_input_memsizeNT(self, text):
        self.input(self.nt_memsize, text)

    def send_input_maxsizeNT(self, text):
        self.input(self.nt_maxsize, text)

    def send_input_cpuNT(self, text):
        self.input(self.nt_cpu, text)

    def send_click_closeNT(self):
        self.click(self.nt_close)

    def send_click_submitNT(self):
        self.click(self.nt_submit)

# to import vm
    def send_click_closeIP(self):
        self.click(self.ip_close)

    def send_click_submitIP(self):
        self.click(self.ip_submit)

# to edit vm
    def send_input_nameE(self, text):
        self.input(self.e_name, text)

    def send_input_macE(self, text):
        self.input(self.e_mac, text)

    def send_input_descriptionE(self, text):
        self.input(self.e_description, text)

    def send_input_memsizeE(self, text):
        self.input(self.e_memsize, text)

    def send_input_maxsizeE(self, text):
        self.input(self.e_maxsize, text)

    def send_input_cpuE(self, text):
        self.input(self.e_cpu, text)

    def send_input_kcpuE(self, text):
        self.input(self.e_kcpu, text)

    def send_input_tcpuE(self, text):
        self.input(self.e_tcpu, text)

    def send_click_closeE(self):
        self.click(self.e_close)

    def send_click_submitE(self):
        self.click(self.e_submit)

# to delete vm
    def send_click_removediskD(self):
        self.click(self.d_removedisk)

    def send_click_unremovediskD(self):
        self.click(self.d_unremovedisk)

    def send_click_cleardiskD(self):
        self.click(self.d_cleardisk)

    def send_click_closeD(self):
        self.click(self.d_close)

    def send_click_submitD(self):
        self.click(self.d_submit)

# to runonce vm
    def send_select_cdRN(self, iso):
        self.click_select_option(self.rn_iso, iso)

    def send_select_bootRN(self, option):
        self.select(self.rn_boot, option)

    def send_click_upRN(self):
        self.click(self.rn_up)

    def send_click_downRN(self):
        self.click(self.rn_down)

    def send_click_closeRN(self):
        self.click(self.rn_close)

    def send_click_submitRN(self):
        self.click(self.rn_submit)

# to migrate vm
    def send_input_bandwidthM(self, text):
        self.input(self.m_bandwidth, text)

    def send_click_closeM(self):
        self.click(self.m_close)

    def send_click_submitM(self):
        self.click(self.m_submit)

# to export vm
    def send_click_closeEP(self):
        self.click(self.ep_close)

    def send_click_submitEP(self):
        self.click(self.ep_submit)

# to turn template
    def send_input_nameT(self, text):
        self.input(self.t_name, text)

    def send_input_descriptionT(self, text):
        self.input(self.t_description, text)

    def send_click_closeT(self):
        self.click(self.t_close)

    def send_click_submitT(self):
        self.click(self.t_submit)

# to take snapshot
    def send_input_nameS(self, text):
        self.input(self.s_name, text)

    def send_input_descriptionS(self, text):
        self.input(self.s_description, text)

    def send_click_closeS(self):
        self.click(self.s_close)

    def send_click_submitS(self):
        self.click(self.s_submit)


# to change cd
    def send_select_cdCD(self, iso):
        self.click_select_option(self.cd_iso, iso)

    def send_click_closeCD(self):
        self.click(self.cd_close)

    def send_click_submitCD(self):
        self.click(self.cd_submit)

# to recovery snapshot
    def send_click_closeRS(self):
        self.click(self.rs_close)

    def send_click_submitRS(self):
        self.click(self.rs_submit)

# to delete snapshot
    def send_click_closeDS(self):
        self.click(self.ds_close)

    def send_click_submitDS(self):
        self.click(self.ds_submit)

# to select vm from table
    def select_vm(self, vm):
        return self.select_table_row(self.table, vm)

# get vm status from table
    def get_vm_status(self, vm):
        return self.get_table_unit_status(self.table, vm)

# to execute result submit
    def send_click_submitR(self):
        self.click(self.r_submit)