/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 145a347e4ba2499fa62b172eceefd8a7a1b19543 */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Colopl_ColoplTimeShifter_register_hook, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, interval, DateInterval, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Colopl_ColoplTimeShifter_unregister_hook, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Colopl_ColoplTimeShifter_is_hooked, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()


ZEND_FUNCTION(Colopl_ColoplTimeShifter_register_hook);
ZEND_FUNCTION(Colopl_ColoplTimeShifter_unregister_hook);
ZEND_FUNCTION(Colopl_ColoplTimeShifter_is_hooked);


static const zend_function_entry ext_functions[] = {
	ZEND_NS_FALIAS("Colopl\\ColoplTimeShifter", register_hook, Colopl_ColoplTimeShifter_register_hook, arginfo_Colopl_ColoplTimeShifter_register_hook)
	ZEND_NS_FALIAS("Colopl\\ColoplTimeShifter", unregister_hook, Colopl_ColoplTimeShifter_unregister_hook, arginfo_Colopl_ColoplTimeShifter_unregister_hook)
	ZEND_NS_FALIAS("Colopl\\ColoplTimeShifter", is_hooked, Colopl_ColoplTimeShifter_is_hooked, arginfo_Colopl_ColoplTimeShifter_is_hooked)
	ZEND_FE_END
};
