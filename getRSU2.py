# -*- coding: utf-8 -*-
#!/usr/bin/env python
def get_road(in_file):
	#读取input_net.net.xml文件
	fr = open(in_file,'r')
	information = fr.readlines()
	fr.close()

	incLanes = 'incLanes=\"'
	count = 0
	d = {}
	for i in range(len(information)):
		line = information[i]
		nPos = line.find(incLanes)
		if nPos >= 0:
			each_part = line.strip().split(' ')
			#获取incLanes的部分
			road = each_part[5]
			junction = each_part[1]
			#去除空的incLanes
			if road == 'incLanes=\"\"':
				continue
				
			road = road[len(incLanes):]
			road = road.rstrip('"')
			junction = junction[4:]
			junction = junction.rstrip('"')
			d[road] = junction
			print junction,road
			count+=1
	print "the number of roads is "+str(count)
	return d

def roadlength(in_file,junctions):
	#读取input_net.net.xml文件
	fr = open(in_file,'r')
	information = fr.readlines()
	fr.close()

	lengths = {}
	lane = '<lane'
	for i in range(len(information)):
		if len(lengths) == len(junctions):
			break

		line = information[i]
		nPos = line.find(lane)
		if nPos >= 0:
			each_part = line.strip().split(' ')
			lane_id = each_part[1][4:-1]
			if lane_id in junctions:
				nPos1 = line.find('length')
				nPos2 = line.find('shape')
				length = line[nPos1+8:nPos2-2]
				lengths[lane_id] = length
				print lane_id,length
			
	return lengths

def get_rsu(junctions,lengths):
	out_file = raw_input("Input OutFile Direction:") 
	first_rsu = raw_input("Input the first ID of RSU:")
	end_time = raw_input("Input the end of time:")
	fw_add = open(out_file,'w')
	add_head = '<additional>'+'\n\t'+'<vType id="RSU" color="255,0,0" guiShape="bus"/>'+'\n\n'
	fw_add.writelines(add_head)

	#设置RSU
	for id in junctions:
		lane = id
		new_id = 'RSU'+junctions[id]
		road = lane[:-2]
		departpos = lengths[lane]
		add_line = '\t<vehicle id="'+new_id+'" type="RSU" depart="0" departPos="'+departpos+'" color="255,0,0">'+'\n\t\t' +'<route edges="'+road+'"/>'+'\n\t\t'+'<stop lane="'+lane+'" parking="true" endPos="'+departpos+'" until="'+end_time+'"/>'+'\n\t</vehicle>\n'
		fw_add.writelines(add_line)
	fw_add.writelines('</additional>')
	fw_add.close()


def main():
	in_file = raw_input("Input File Direction:")
	junctions = get_road(in_file)
	lengths = roadlength(in_file,junctions)
	get_rsu(junctions,lengths)

if __name__ == '__main__':
	main()