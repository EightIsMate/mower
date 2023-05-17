import requests
import time

IMG_URL = "https://ims8.herokuapp.com/upload"
POS_URL = "https://ims8.herokuapp.com/positions/mover"
	
def upload_positions(payload):
	req_pos = requests.request("POST", POS_URL, auth = ("username", "password"), data = payload)
	print("Status: ", req_pos.status_code, "req_pos.txt = ", req_pos.text)
	return req_pos.json()["id"]
	
def upload_image_to_api(image, payload):
	image_file = [('file',('myfile.jpg',open(image,'rb'),'image/jpeg'))]
	req = requests.request("POST", IMG_URL, auth = ("username", "password"), data = {"positionid": upload_positions(payload)}, files = image_file)
	print("Status: ", req.status_code, "req.txt = ", req.text)
	number_of_checks = 0
	recheck = int(time.time())+ 15