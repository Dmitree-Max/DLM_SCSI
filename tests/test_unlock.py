from supporting_functions import *


def test_unlock():
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

    check_state_on_nodes(keys, locks, all_nodes)

    lock_key(other_node, "test2_key")
    locks.append("test2_key")
    locks.append(other_node)

    check_state_on_nodes(keys, locks, all_nodes)

    # unlock from wrong node
    unlock_key(this_node, "test2_key")

    check_state_on_nodes(keys, locks, all_nodes)

    # unlock of unlocked value
    unlock_key(other_node, "test_key")

    check_state_on_nodes(keys, locks, all_nodes)

    # unlock of unlocked value
    unlock_key(this_node, "test_key")

    check_state_on_nodes(keys, locks, all_nodes)

    # unlock of not existing value
    unlock_key(this_node, "test3_key")

    check_state_on_nodes(keys, locks, all_nodes)

    # right unlock
    unlock_key(this_node, "test2_key")
    locks.pop()
    locks.pop()

    check_state_on_nodes(keys, locks, all_nodes)

    # unlock of unlocked
    unlock_key(this_node, "test2_key")  

    check_state_on_nodes(keys, locks, all_nodes)

