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

    check_state_on_nodes(keys, locks, all_nodes)

    add_key(other_node, "test3_key", "test3_value")
    keys.append("test3_key")
    keys.append("test3_value")

    check_state_on_nodes(keys, locks, all_nodes)