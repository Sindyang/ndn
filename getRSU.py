# -*- coding: utf-8 -*-
#!/usr/bin/env python
import time

def get_road(in_file):
	#读取input_net.net.xml文件
	fr = open(in_file,'r')
	information = fr.readlines()
	fr.close()

	edge = '<edge'
	count = 0
	roads = []
	for i in range(len(information)):
		line = information[i]
		nPos = line.find(edge)
		if nPos >= 0:
			nId = line.find('"')
			#保存所有道路信息
			roads.append(line[nId+1:nId+9])
			count+=1
	print "the number of road is "+ str(count)
	return roads
		

def get_rsu(roads,in_file):
	#读取trips.trips.xml文件
	fr = open(in_file,'r')
	i = 0
	#忽略无关内容
	for i in range(5):
		fr.readline()
	information = fr.readlines()
	fr.close()

	count1 = 0
	count2 = 0
	out_file_trip = 'E:/ndn_input_data/input_125_RSU_48/newtrips.trips.xml'
	out_file_add = 'E:/ndn_input_data/input_125_RSU_48/newtype.add.xml'
	fw_trip = open(out_file_trip,'w')
	fw_add = open(out_file_add,'w')
	trip_head = '<?xml version="1.0"?>'+'\n'+'<!-- generated on '+time.strftime('%Y-%m-%d %H:%M:%S',time.localtime(time.time())) +' by $Id: randomTrips.py 18756 2015-08-31 19:16:33Z behrisch $'+'\n'+'  options: -n input_net.net.xml -e 1.2 -p 0.01 <doubleminus>trip-attributes=departLane="best"'+'\n'+'-->'+'\n'+'<trips>'+'\n'
	fw_trip.writelines(trip_head)

	add_head = '<additional>'+'\n\t'+'<vType id="BUS" color="1,1,1" guiShape="bus"/>'+'\n\n'
	fw_add.writelines(add_head)

	for i in range(len(information)-1):
		line = information[i]
		each_part = line.strip().split(' ')
		road = each_part[3][6:-1]
		if road in roads:
			vehicle_id = each_part[1]
			#这里修改停止的位置和时间
			add_line = '\t<vehicle '+vehicle_id+' type="BUS" depart="0" departPos="500" color="1,1,1">'+'\n\t\t' +'<route edges="'+road+'"/>'+'\n\t\t'+'<stop lane="'+road+'_0" endPos="500" until="400"/>'+'\n\t</vehicle>\n'
			fw_add.writelines(add_line)
			roads.remove(road)
			count1 += 1
		else:
			#each_part[2] = 'depart="0.00"'
			line = '\t'+each_part[0]+' '+each_part[1]+' '+each_part[2]+' '+each_part[3]+' '+each_part[4]+' '+each_part[5]+'\n'
			fw_trip.writelines(line)
			count2 += 1

	if len(roads) == 0:
		print "All of roads have found RSU"
	else:
		print "These roads doesn't have RSU"+'\n'
		i = 0
		for i in range(len(roads)):
			print roads[i]
	
	#为没有RSU的道路添加RSU
	total = count1+count2
	for new_vehicle_id in range(total,total+i+1):
		#这里修改停止的位置和时间
		add_line = '\t<vehicle id="'+ str(new_vehicle_id) +'" type="BUS" depart="0" departPos="500" color="1,1,1">'+'\n\t\t' +'<route edges="'+roads[0]+'"/>'+'\n\t\t'+'<stop lane="'+roads[0] +'_0" endPos="500" until="400"/>'+'\n\t</vehicle>\n'
		fw_add.writelines(add_line)
		roads.remove(roads[0])
		
		
	if len(roads) == 0:
		print "At last, All of roads have found RSU"
	else:
		print "These roads doesn't have RSU"+'\n'
		i = 0
		for i in range(len(roads)):
			print roads[i]
	 
		
	fw_trip.writelines('</trips>')
	fw_trip.close()
	fw_add.writelines('</additional>')
	fw_add.close()
	
	
def main():
	in_file1 = 'E:/ndn_input_data/input_125_RSU_48/input_net.net.xml'
	in_file2 = 'E:/ndn_input_data/input_125_RSU_48/trips.trips.xml'
	roads = get_road(in_file1) 
	get_rsu(roads,in_file2)

if __name__ == '__main__':
    main()