from supporting_functions import *


def test_add():
    assert_state_is_empty()
    locks = []
    keys = []

    for i in range(100):
        add_key(this_node, "key" + str(i), "value" + str(i))
        keys.append("key" + str(i))
        keys.append("value" + str(i))

        check_state_on_nodes(keys, locks, all_nodes)

