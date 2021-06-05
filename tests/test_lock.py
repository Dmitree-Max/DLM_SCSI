from supporting_functions import *


def test_lock():
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

    # existing key
    lock_key(other_node, "test2_key")
    locks.append("test2_key")
    locks.append(node_names[other_node])

    check_state_on_nodes(keys, locks, all_nodes)

    # not existing key
    lock_key(other_node, "test3_key")

    check_state_on_nodes(keys, locks, all_nodes)

    # lock of locked value
    lock_key(other_node, "test2_key")

    check_state_on_nodes(keys, locks, all_nodes)

    # lock of locked value by other node
    lock_key(this_node, "test2_key")

    check_state_on_nodes(keys, locks, all_nodes)

    clean_state()

