from supporting_functions import *


def test_this_node():
    assert_state_is_empty()
    execute_command_ckv(this_node, 'af asdf sadf')
    assert_state_is_empty()

    # right is add-key
    execute_command_ckv(this_node, 'addkey asdf sadf')
    assert_state_is_empty()

    # add-key has no arguments
    execute_command_ckv(this_node, 'addkey')
    assert_state_is_empty()

    # add-key has only one argument
    execute_command_ckv(this_node, 'addkey asdf')
    assert_state_is_empty()


def test_other_node():
    assert_state_is_empty()
    execute_command_ckv(other_node, 'af asdf sadf')
    assert_state_is_empty()

    # right is add-key
    execute_command_ckv(other_node, 'addkey asdf sadf')
    assert_state_is_empty()

    # add-key has no arguments
    execute_command_ckv(other_node, 'addkey')
    assert_state_is_empty()

    # add-key has only one argument
    execute_command_ckv(other_node, 'addkey asdf')
    assert_state_is_empty()
