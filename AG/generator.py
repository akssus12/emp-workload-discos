from random import randint

max_value = 10
iter = 20

key = [randint(0, max_value-1) for p in range(0, iter)]
value = [randint(1, max_value) for p in range(0, iter)]

f = open("text.txt", 'w')
f.write("{}\n".format(max_value))
for i in range(iter):
    data = "<{},{}>\n".format(key[i], value[i])
    f.write(data)