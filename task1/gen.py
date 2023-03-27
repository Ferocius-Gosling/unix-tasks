size = 4 * 1024 * 1024 + 1

with open("fileA", "w+") as file:
	for i in range(size):
		if i == 0 or i == 10000 or i == size - 1:
			file.write('\x01')
		else:
			file.write('\x00')
