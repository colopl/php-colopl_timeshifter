/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 328c02a318c199e599e76258e71fb99fc9288198 */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Colopl_ColoplTimeShifter_register_hook, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, interval, DateInterval, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Colopl_ColoplTimeShifter_unregister_hook, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Colopl_ColoplTimeShifter_is_hooked, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Colopl_ColoplTimeShifter_Manager_isAvailable arginfo_Colopl_ColoplTimeShifter_is_hooked

#define arginfo_class_Colopl_ColoplTimeShifter_Manager_isHooked arginfo_Colopl_ColoplTimeShifter_is_hooked

#define arginfo_class_Colopl_ColoplTimeShifter_Manager_unhook arginfo_Colopl_ColoplTimeShifter_is_hooked

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Colopl_ColoplTimeShifter_Manager_hookDateTime, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, dateTime, DateTimeInterface, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Colopl_ColoplTimeShifter_Manager_hookDateInterval, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, dateInterval, DateInterval, 0)
ZEND_END_ARG_INFO()

ZEND_FUNCTION(Colopl_ColoplTimeShifter_register_hook);
ZEND_FUNCTION(Colopl_ColoplTimeShifter_unregister_hook);
ZEND_FUNCTION(Colopl_ColoplTimeShifter_is_hooked);
ZEND_METHOD(Colopl_ColoplTimeShifter_Manager, isAvailable);
ZEND_METHOD(Colopl_ColoplTimeShifter_Manager, isHooked);
ZEND_METHOD(Colopl_ColoplTimeShifter_Manager, unhook);
ZEND_METHOD(Colopl_ColoplTimeShifter_Manager, hookDateTime);
ZEND_METHOD(Colopl_ColoplTimeShifter_Manager, hookDateInterval);

static const zend_function_entry ext_functions[] = {
#if (PHP_VERSION_ID >= 80400)
	ZEND_RAW_FENTRY(ZEND_NS_NAME("Colopl\\ColoplTimeShifter", "register_hook"), zif_Colopl_ColoplTimeShifter_register_hook, arginfo_Colopl_ColoplTimeShifter_register_hook, 0, NULL, NULL)
#else
	ZEND_RAW_FENTRY(ZEND_NS_NAME("Colopl\\ColoplTimeShifter", "register_hook"), zif_Colopl_ColoplTimeShifter_register_hook, arginfo_Colopl_ColoplTimeShifter_register_hook, 0)
#endif
#if (PHP_VERSION_ID >= 80400)
	ZEND_RAW_FENTRY(ZEND_NS_NAME("Colopl\\ColoplTimeShifter", "unregister_hook"), zif_Colopl_ColoplTimeShifter_unregister_hook, arginfo_Colopl_ColoplTimeShifter_unregister_hook, 0, NULL, NULL)
#else
	ZEND_RAW_FENTRY(ZEND_NS_NAME("Colopl\\ColoplTimeShifter", "unregister_hook"), zif_Colopl_ColoplTimeShifter_unregister_hook, arginfo_Colopl_ColoplTimeShifter_unregister_hook, 0)
#endif
#if (PHP_VERSION_ID >= 80400)
	ZEND_RAW_FENTRY(ZEND_NS_NAME("Colopl\\ColoplTimeShifter", "is_hooked"), zif_Colopl_ColoplTimeShifter_is_hooked, arginfo_Colopl_ColoplTimeShifter_is_hooked, 0, NULL, NULL)
#else
	ZEND_RAW_FENTRY(ZEND_NS_NAME("Colopl\\ColoplTimeShifter", "is_hooked"), zif_Colopl_ColoplTimeShifter_is_hooked, arginfo_Colopl_ColoplTimeShifter_is_hooked, 0)
#endif
	ZEND_FE_END
};

static const zend_function_entry class_Colopl_ColoplTimeShifter_Manager_methods[] = {
	ZEND_ME(Colopl_ColoplTimeShifter_Manager, isAvailable, arginfo_class_Colopl_ColoplTimeShifter_Manager_isAvailable, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(Colopl_ColoplTimeShifter_Manager, isHooked, arginfo_class_Colopl_ColoplTimeShifter_Manager_isHooked, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(Colopl_ColoplTimeShifter_Manager, unhook, arginfo_class_Colopl_ColoplTimeShifter_Manager_unhook, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(Colopl_ColoplTimeShifter_Manager, hookDateTime, arginfo_class_Colopl_ColoplTimeShifter_Manager_hookDateTime, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(Colopl_ColoplTimeShifter_Manager, hookDateInterval, arginfo_class_Colopl_ColoplTimeShifter_Manager_hookDateInterval, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_Colopl_ColoplTimeShifter_Manager(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Colopl\\ColoplTimeShifter", "Manager", class_Colopl_ColoplTimeShifter_Manager_methods);
#if (PHP_VERSION_ID >= 80400)
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, ZEND_ACC_FINAL);
#else
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_FINAL;
#endif

	return class_entry;
}
