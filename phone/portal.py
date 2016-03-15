import pprint
import time
import requests

auth = ('', '')
url_upload = 'https://swapfile.asuscomm.com/portalguns/add.php'
url_goron_data ='http://192.168.1.22/tmp/portal.txt'
url_chell_data ='http://192.168.1.23/tmp/portal.txt'
url_goron_image = 'http://192.168.1.22/tmp/snapshot.jpg'
url_chell_image = 'http://192.168.1.23/tmp/snapshot.jpg'

session_gordon = requests.Session()
session_chell = requests.Session()
session_upload = requests.Session()

#get file from gun 1
gordon_text = session_gordon.get(url_goron_data).content.split( )
gordon = {};
gordon['state_solo'] = int(gordon_text[0])
gordon['state_duo'] = int(gordon_text[1])
gordon['connected'] = bool(gordon_text[2])
gordon['battery_level'] = float(gordon_text[26])
gordon['temp'] = float(gordon_text[27])
gordon['core_temp'] = float(gordon_text[28])
gordon['latency'] = float(gordon_text[29])
gordon['packet_counter'] = int(gordon_text[30])

#get file from gun 2
chell_text = session_chell.get(url_chell_data).content.split( )
chell = {};
chell['state_solo'] = int(chell_text[0])
chell['state_duo'] = int(chell_text[1])
chell['connected'] = bool(chell_text[2])
chell['battery_level'] = float(chell_text[26])
chell['temp'] = float(chell_text[27])
chell['core_temp'] = float(chell_text[28])
chell['latency'] = float(chell_text[29])
chell['packet_counter'] = int(chell_text[30])

#determine which gun if any to get image from
pprint.pprint(chell)
pprint.pprint(gordon)

payload = {};

if chell['state_duo'] < 0:
	payload['img'] = ('i.jpg', session_chell.get(url_chell_image).content)
if gordon['state_duo'] < 0:
	payload['img'] = ('i.jpg', session_gordon.get(url_goron_image).content)

#post data to server
payload['csv'] = ('r.csv', 'some,data,to,send')

upload_result = session_upload.post(url_upload, auth=auth,files=payload)
upload_result =  str( upload_result.content, encoding=upload_result.encoding ) 
print (upload_result)