from urllib import request

code = '998407.O'
base_url = 'https://finance.yahoo.co.jp/quote/'

url = base_url + code
response = request.urlopen(url)
content = response.read()
response.close()
html = content.decode()

title = html.split('<span class="_1Eg3JlPA">')[1].split('</span')[0]
price = html.split('<span class="_3bYcwOHa">')[1].split('</span>')[0]
print('%s:\t%s円' % (title, price) )

