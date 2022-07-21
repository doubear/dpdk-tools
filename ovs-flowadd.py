#! /usr/bin/python3
# SPDX-License-Identifier: BSD-3-Clause
# copyright by doubear
import os

def addflow():
    print("input bridge first")
    print("such as br0 ")
    br = input()
    os.system("ovs-ofctl show " + br)
    os.system("ovs-ofctl dump-flows " + br)
    print("+" * 100)
    print("such as 1 2 10.0.0.1 10.1.0.1 10")
    print("input in_port, output, ipstart, ipdst, flownum")
    in_port,output,ipstart,ipdst,flownum = map(str,input().split())
    #print(br,in_port,output,ipsatrt,ipdst,flownum)
    stradd = "ovs-ofctl add-flow " + br + " "
    proto = "dl_type=0x0800"
    ip1 = [int(i) for i in ipdst.split(".",-1)]    
    ip2 = [int(i) for i in ipstart.split(".",-1)] 
    
    if ipdst != "0" and ipstart == "0":
        for num in range(int(flownum)):
            if ip1[3] < 255:
                os.system(stradd + proto + ",in_port=" +
                in_port + ",nw_dst=" + str(ip1[0]) + "." + 
                str(ip1[1]) + "." + str(ip1[2]) + "." + str(ip1[3]) +
                ",action=output:" + output) 
                ip1[3] += 1 
            else:
                ip1[2] += 1
                ip1[3] = 1
        print("+" * 100)
    elif ipdst == "0" and ipstart != "0":
        for num in range(int(flownum)):
            if ip2[3] < 255:
                os.system(stradd + proto + ",in_port=" +
                in_port + ",nw_src=" + str(ip2[0]) + "." +
                str(ip2[1]) + "." + str(ip2[2]) + "." + str(ip2[3]) +
                ",action=output:" + output) 
                ip2[3] += 1 
            else:
                ip2[2] += 1
                ip2[3] = 1
        print("+" * 100)
    else:
        for num in range(int(flownum)):
            if ip1[3] < 255:
                os.system(stradd + proto + ",in_port=" +
                in_port + ",nw_src=" + str(ip2[0]) + "." +
                str(ip2[1]) + "." + str(ip2[2]) + "." + str(ip2[3]) +
                ",nw_dst=" + str(ip1[0]) + "." +
                str(ip1[1]) + "." + str(ip1[2]) + "." + str(ip1[3]) +
                ",action=output:" + output) 
                ip1[3] += 1 
                ip2[3] += 1
            else:
                ip2[2] += 1
                ip2[3] = 1
                ip1[2] += 1
                ip1[3] = 1
        print("+" * 100)
    os.system("ovs-ofctl dump-flows " + br)
    

def delflow():
    print("input bridge first")
    print("such as br0 ")
    br = input()
    os.system("ovs-ofctl show " + br)
    os.system("ovs-ofctl dump-flows " + br)
    
    print("ipstart and ipdst flownum such as 10.0.0.1 10.1.0.1 10 ")
    ipstart,ipdst,flownum = map(str,input().split())
    strdel = "ovs-ofctl del-flows " + br + " "
    proto = "dl_type=0x0800"
    ip1 = [int(i) for i in ipdst.split(".",-1)]
    ip2 = [int(i) for i in ipstart.split(".",-1)]

    if ipstart == "0" and ipdst != "0":
        for num in range(int(flownum)):
            if ip1[3] < 255: 
                os.system(strdel + proto + ",nw_dst=" + str(ip1[0]) + "." +
                str(ip1[1]) + "." + str(ip1[2]) + "." + str(ip1[3]) ) 
                ip1[3] += 1 
            else:
                ip1[2] += 1
                ip1[3] = 1
        print("+" * 100)
    elif ipdst == "0" and ipstart != "0":
        for num in range(int(flownum)):
            if ip2[3] < 255: 
                os.system(strdel + proto + ",nw_src=" + str(ip2[0]) + "." +
                str(ip2[1]) + "." + str(ip2[2]) + "." + str(ip2[3]) ) 
                ip2[3] += 1
            else:
                ip2[2] += 1
                ip2[3] = 1
        print("+" * 100)
    else:
        for num in range(int(flownum)):
            if ip1[3] < 255:
                os.system(strdel + proto + ",nw_src=" + str(ip2[0]) + "." +
                str(ip2[1]) + "." + str(ip2[2]) + "." + str(ip2[3]) +
                ",nw_dst=" + str(ip1[0]) + "." + str(ip1[1]) + "." + str(ip1[2]) + "." + str(ip1[3]) ) 
                ip1[3] += 1 
                ip2[3] += 1
            else:
                ip2[2] += 1
                ip2[3] = 1
                ip1[2] += 1
                ip1[3] = 1
        print("+" * 100)
    os.system("ovs-ofctl dump-flows " + br) 

def addflow_with_pri():
    print("support in the future")
    pass

def addflow_with_table():
    print("support in the future")
    pass


def main():
    os.system("ovs-vsctl show")
    n = 0
    while n < 3:
        print("input: 1、addflow mode")
        print("       2、delflow mode")
        print("       3、addflow with priority mode")
        print("       4、addflow with table num mode")
        mode =  input() 
        if mode == "1":
            addflow()
            break
        elif mode == "2":
            delflow()
            break
        elif mode == "3":
            addflow_with_pri()
        elif mode == "4":
            addflow_with_table() 
        else:
            print("input error")
        n += 1

if __name__ == "__main__":
    main()
