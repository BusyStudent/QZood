import requests
import random

url = 'https://www.yhdmz2.com/vp/22582-2-0.html'
def gen_request_addr(url : str):
    parts = url.split("/")[-1].split('-')

    # It shoud be 3
    if len(parts) != 3:
        return ""
    
    u = 'playurl?aid={}&playindex={}&epindex={}&r={}'.format(parts[0], parts[1], parts[2].split('.')[0], str(random.random()))

    return 'https://www.yhdmz2.com/' + u
def parse_data(data : str):
    output = ""
    magic = 1561
    length = len(data)
    for cur in range(0, length, 2):
        ret = int(data[cur] + data[cur + 1], 16)
        ret = (((ret + 1048576) - magic) - (((length / 2) - 1) - (cur / 2))) % 256
        output = chr(int(ret)) + output
    return output

print(gen_request_addr(url))
u = gen_request_addr(url)

header = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.142.86 Safari/537.36",
    "Referer": url
}
season = requests.Session()

req = season.get(u, headers=header)
data = None
for i in range(0, 3) :
    if req.ok:
        # print(req.content)
        # print(season.cookies)
        if req.content.decode().startswith('ipchk') :
            req = season.get(u, headers=header)
            continue
        else:
            data = req.content.decode()
            break
    else:
        print(req.status_code)
        break
if data != None:
    print(parse_data(data))

