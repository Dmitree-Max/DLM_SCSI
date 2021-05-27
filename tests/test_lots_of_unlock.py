from supporting_functions import *


def test_add():
    assert_state_is_empty()
    locks = []
    keys = []

    add_key(this_node, "test_key", "test_value")
    keys.append("test_key")
    keys.append("test_value")

    check_state_on_nodes(keys, locks, all_nodes)

    for i in range(100):
        lock_key(other_node, "test_key")
        locks.append("test_key")
        locks.append(other_node)

        check_state_on_nodes(keys, locks, all_nodes)

        unlock_key(other_node, "test_key")
        locks.pop()
        locks.pop()

        check_state_on_nodes(keys, locks, all_nodes)

