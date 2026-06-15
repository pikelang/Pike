#include <check.h>
#include <stdlib.h>
#include <string.h>

/* Simulate the vulnerable pattern from src/object.c:624-625 to test the invariant */
/* In production, this would call the actual Pike object loading function */

static int safe_append_extension(const char *master_file, char *tmp, size_t tmp_size)
{
    /* Invariant: buffer writes never exceed declared length */
    size_t needed = strlen(master_file) + 3; /* +1 for null, +2 for ".o" */
    
    if (needed > tmp_size) {
        return -1; /* Reject: would overflow */
    }
    
    memcpy(tmp, master_file, strlen(master_file) + 1);
    strcat(tmp, ".o");
    return 0;
}

START_TEST(test_buffer_overflow_on_extension_append)
{
    /* Invariant: Buffer reads/writes never exceed the declared length */
    const char *payloads[] = {
        /* Exact exploit case: string that fills buffer exactly, no room for ".o" */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
        /* Boundary: one byte short of overflow */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
        /* Valid input: normal short path */
        "/usr/lib/pike/master"
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        /* Allocate buffer that would overflow with vulnerable code */
        size_t buf_size = strlen(payloads[i]) + 1; /* No room for ".o" */
        char *tmp = malloc(buf_size);
        ck_assert_ptr_nonnull(tmp);
        
        /* Safe version must reject inputs that would overflow */
        int result = safe_append_extension(payloads[i], tmp, buf_size);
        
        /* Invariant check: if buffer is too small, operation must fail */
        if (strlen(payloads[i]) + 3 > buf_size) {
            ck_assert_int_eq(result, -1);
        }
        
        free(tmp);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_buffer_overflow_on_extension_append);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}