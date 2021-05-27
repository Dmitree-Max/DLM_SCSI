import paramiko
import os
import time

other_node = "10.10.10.9"
user = "dmitree"
this_node = "10.10.10.8"
filepath = "/dev/ckv"
password = "irfa00vtn"
all_nodes = [other_node, this_node]


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
    time.sleep(3)


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