from supporting_functions import *


def test_add():
    assert_state_is_empty()
    locks = []
    keys = []

    add_key(this_node, "test_key", "test_value")
    keys.append("test_key")
    keys.append("test_value")

    check_state_on_nodes(keys, locks, all_nodes)

    add_key(this_node, "test2_key", "test2_value")
    keys.append("test2_key")
    keys.append("test2_value")

    lock_key(other_node, "test2_key")
    locks.append("test2_key")
    locks.append(other_node)

    check_state_on_nodes(keys, locks, all_nodes)

    # this lock is taken
    lock_key(this_node, "test2_key")

    check_state_on_nodes(keys, locks, all_nodes)

    # failed again
    lock_key(this_node, "test2_key")

    check_state_on_nodes(keys, locks, all_nodes)

    # other node released lock
    unlock_key(other_node, "test2_key")
    locks.pop()
    locks.pop()

    check_state_on_nodes(keys, locks, all_nodes)

    # this node finally take lock
    lock_key(this_node, "test2_key")
    locks.append("test2_key")
    locks.append(this_node)

    check_state_on_nodes(keys, locks, all_nodes)
