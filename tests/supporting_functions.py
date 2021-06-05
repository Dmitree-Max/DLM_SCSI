import paramiko
import os
import time

other_node = "10.10.10.9"
user = "dmitree"
this_node = "10.10.10.8"
filepath = "/dev/ckv"
password = "irfa00vtn"
# all_nodes = [other_node, this_node]
all_nodes = [this_node]

node_names = {this_node: "1", other_node: "2"}


def get_address_by_name(name):
    address = ""
    for key in node_names.keys():
        if node_names[key] == name:
            address = key
            break

    assert address != ""
    return address


def work_with_file_read(function_to_do):
    def wrapper(node_addr, *args, **kwargs):
        if node_addr == this_node:
            with open(filepath) as fp:
                return function_to_do(fp, *args, **kwargs)
        else:
            ssh = paramiko.SSHClient()
            ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            ssh.connect(node_addr, username=user, password=password)
            ssh.exec_command('cp ' + filepath + ' /tmp/ckv')
            sftp = ssh.open_sftp()
            with sftp.open('/tmp/ckv', 'r') as fp:
                return function_to_do(fp, *args, **kwargs)
            ssh.close()
    return wrapper


def execute_command_bash(node_addr, command):
    if node_addr == this_node:
        os.system(command)
    else:
        ssh = paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        ssh.connect(node_addr, username=user, password=password)
        ssh.exec_command(command)
        ssh.close()


def execute_command_ckv(node_addr, command):
    if node_addr == this_node:
        os.system('echo ' + command + ' > /dev/ckv')
    else:
        ssh = paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        ssh.connect(node_addr, username=user, password=password)
        ssh.exec_command('echo ' + command + ' > /dev/ckv')
        ssh.close()

    # We need to wait after each command to test that change made on all nodes
    time.sleep(1)


def compare_lists(x, y):
    assert len(x) == len(y)
    for i in range(len(x)):
        assert x[i] == y[i]
    return


@work_with_file_read
def read_keys_and_locks_from_file(fp):
    keys = []
    locks = []

    line = fp.readline()
    assert line == 'keys:\n'
    while line:
        line = fp.readline()
        if '=' in line:
            key, value = line.split('=')
            key = key.replace(" ", "")
            value = value.replace(" ", "")
            value = value.replace("\n", "")
            keys.append(key)
            keys.append(value)
        else:
            assert line == "\n"
            break

    line = fp.readline()
    assert line == 'locks:\n'
    while line:
        line = fp.readline()
        if '(' in line:
            lock, node = line.split('(')
            node = node.replace(")", "")
            lock = lock.replace(" ", "")
            node = node.replace(" ", "")
            node = node.replace("\n", "")
            locks.append(lock)
            locks.append(node)
        else:
            break
    return keys, locks


def add_key(nodeaddr, key, value):
    execute_command_ckv(nodeaddr, "add-key " + key + " " + value)


def lock_key(nodeaddr, key):
    execute_command_ckv(nodeaddr, "lock-key " + key)


def remove_key(nodeaddr, key):
    execute_command_ckv(nodeaddr, "remove-key " + key)


def unlock_key(nodeaddr, key):
    execute_command_ckv(nodeaddr, "unlock-key " + key)


def assert_state_is_empty():
    ks, ls = read_keys_and_locks_from_file(this_node)
    compare_lists(ks, [])
    compare_lists(ls, [])
    ks, ls = read_keys_and_locks_from_file(other_node)
    compare_lists(ks, [])
    compare_lists(ls, [])


def check_state_on_nodes(keys, locks, nodes):
    for node in nodes:
        ks, ls = read_keys_and_locks_from_file(node)
        compare_lists(keys, ks)
        compare_lists(locks, ls)


def remove_all_locks():
    _, ls = read_keys_and_locks_from_file(this_node)
    assert len(ls) % 2 == 0
    while len(ls) > 1:
        key = ls[0]
        lock_owner = ls[1]
        unlock_key(get_address_by_name(lock_owner), key)
        ls.pop(0)
        ls.pop(0)


def clean_state():
    remove_all_locks()
    ks, ls = read_keys_and_locks_from_file(this_node)
    assert len(ls) == 0
    assert len(ks) % 2 == 0

    while len(ks) > 1:
        key = ks[0]
        remove_key(this_node, key)
        ks.pop(0)
        ks.pop(0)

    ks, ls = read_keys_and_locks_from_file(this_node)
    assert len(ks) == 0
    assert len(ls) == 0

