'''
makefont.py: generate hankaku char array in c code from hankaku.txt
'''

if __name__ == '__main__':
	fd = open('hankaku.txt', 'r')
	hankaku_list = {}
	cur_char = 0x00
	for line in fd.readlines():
		if line.startswith('char'):
			cur_char = eval(line.split(' ')[1].strip())
			continue
		if len(line) < 8: continue
		bin_num = line.replace('.', '0').replace('*', '1').strip()
		hankaku_list.setdefault(cur_char, [])
		hankaku_list[cur_char].append(eval('0b'+bin_num))
	fd.close()
	
	c_code = '''
char hankaku[4096] = {
'''
	for key in sorted(hankaku_list.keys()):
		c_code += '    '	
		for code in hankaku_list[key]:
			c_code += '0x%02x' % code + ', '
		c_code += '\n'
	
	c_code = c_code[:c_code.rfind(',')] + '\n'
	c_code += '};'
	
	fd = open('hankaku.h', 'w')
	fd.write(c_code)
	fd.close()
