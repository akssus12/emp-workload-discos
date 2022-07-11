from random import randint

max_value = 10000
key = [randint(0, max_value) for p in range(0, 100)]
value = [randint(1, max_value) for p in range(0, 100)]

f = open("text.txt", 'w')
f.write("{}\n".format(max_value))
for i in range(100):
    data = "<{},{}> ".format(key[i], value[i])
    f.write(data)