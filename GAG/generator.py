from random import randint

max_value = 1000000
key = [randint(0, max_value) for p in range(0, 100000000)]
value = [randint(1, max_value) for p in range(0, 100000000)]

f = open("text.txt", 'w')
f.write("{}\n".format(max_value))
for i in range(100000000):
    data = "<{},{}>\n".format(key[i], value[i])
    f.write(data)