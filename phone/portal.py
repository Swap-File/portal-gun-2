import pprint
import time
from datetime import datetime
import requests

auth = ('', '')
url_upload = 'https://swapfile.asuscomm.com/portalguns/add.php'
url_goron_data ='http://192.168.1.22/tmp/portal.txt'
url_chell_data ='http://192.168.1.23/tmp/portal.txt'
url_goron_image = 'http://192.168.1.22/tmp/snapshot.jpg'
url_chell_image = 'http://192.168.1.23/tmp/snapshot.jpg'

gordon_packet_id_last = -1
chell_packet_id_last = -1
gordon_last_seen_time = -1
chell_last_seen_time = -1

session_gordon = requests.Session()
session_chell = requests.Session()
session_upload = requests.Session()

cycle = 0
while (cycle < 2):
	payload = {};

	#get file from gun 1
	gordon_text = session_gordon.get(url_goron_data).content.split( )

	gordon_packet_counter = int(gordon_text[30])
	if (gordon_packet_counter != gordon_packet_id_last and gordon_packet_id_last != -1):
		payload['g_state'] = int(gordon_text[1])
		gordon_syned = int(gordon_text[2])
		payload['g_volts'] = float(gordon_text[26])
		payload['g_temp1'] = float(gordon_text[27])
		payload['g_temp2'] = float(gordon_text[28])
		payload['g_lag'] = float(gordon_text[29])
		payload['g_live'] = 1
	else:
		payload['g_state'] = 0
		gordon_syned = 0
		payload['g_volts'] = 0
		payload['g_temp1'] = 0
		payload['g_temp2'] = 0
		payload['g_lag'] = 0
		payload['g_live'] = 0
	gordon_packet_id_last = gordon_packet_counter	

	#get file from gun 2
	chell_text = session_chell.get(url_chell_data).content.split( )
	
	chell_packet_counter = int(chell_text[30])
	if (chell_packet_counter != chell_packet_id_last and chell_packet_id_last != -1):
		payload['c_state'] = int(chell_text[1])
		chell_synced = int(chell_text[2])
		payload['c_volts'] = float(chell_text[26])
		payload['c_temp1'] = float(chell_text[27])
		payload['c_temp2'] = float(chell_text[28])
		payload['c_lag'] = float(chell_text[29])
		payload['c_live'] = 1
	else:
		payload['c_state'] = 0
		chell_synced = 0
		payload['c_volts'] = 0
		payload['c_temp1'] = 0
		payload['c_temp2'] = 0
		payload['c_lag'] = 0
		payload['c_live'] = 0
	chell_packet_id_last = chell_packet_counter

	if (chell_synced == 1 and gordon_syned == 1):
		payload['synced'] = 1
	else:
		payload['synced'] = 0
		
	#determine which gun if any to get image from
	files = {};
	if payload['c_state'] < 0:
		files['img'] = ('i.jpg', session_chell.get(url_chell_image).content)
			
	if payload['g_state'] < 0:
		files['img'] = ('i.jpg', session_gordon.get(url_goron_image).content)
		
	#supress warmup cycle
	if cycle > 0:
		upload_result = session_upload.post(url_upload,data=payload, auth=auth,files=files)
		upload_result = str( upload_result.content, encoding=upload_result.encoding ) 
		print (upload_result)
		
	cycle += 1
	time.sleep(2)