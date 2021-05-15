import random

file_prefix = "test_data_"


def generate_numbers(n):
	numbers = [str(i) for i in range(n)]
	random.shuffle(numbers)
	return '\n'.join(numbers)


def main():
	global file_prefix
	for indx in range(7):
		with open(file_prefix + str(indx), 'w') as f:
			f.write(generate_numbers(10 ** indx))


if __name__ == '__main__':
	main()
