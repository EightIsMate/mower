import requests
import time
	
def upload_positions(pos_url, payload):
	req_pos = requests.request("POST", pos_url, auth = ("username", "password"), data = payload)
	print("Status: ", req_pos.status_code, "req_pos.txt = ", req_pos.text)
	return req_pos.json()["id"]
	
def upload_image_to_api(pos_url,url, image, payload):
	image_file = [('file',('myfile.jpg',open(image,'rb'),'image/jpeg'))]
	req = requests.request("POST", url, auth = ("username", "password"), data = {"positionid": upload_positions(pos_url, payload)}, files = image_file)
	print("Status: ", req.status_code, "req.txt = ", req.text)
	number_of_checks = 0
	recheck = int(time.time())+ 15
