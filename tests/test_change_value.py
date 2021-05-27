from supporting_functions import *


def test_add():
    assert_state_is_empty()
    locks = []
    keys = []

    add_key(this_node, "test_key", "test_value")
    keys.append("test_key")
    keys.append("test_value")

    check_state_on_nodes(keys, locks, all_nodes)

    add_key(this_node, "test_key", "changed_value")
    keys[1] = "changed_value"

    check_state_on_nodes(keys, locks, all_nodes)

    add_key(other_node, "test_key", "changed2_value")
    keys[1] = "changed2_value"

    check_state_on_nodes(keys, locks, all_nodes)