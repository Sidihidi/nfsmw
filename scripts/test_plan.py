"""Regressions for DWARF function boundaries; no game binaries required."""
import io
import unittest
from unittest import mock

import plan


class FunctionBlockTests(unittest.TestCase):
    def read_block(self, text, offset=0):
        with mock.patch.object(plan.io, 'open', return_value=io.StringIO(text)):
            return plan.read_fn_block(offset)

    def test_empty_declaration_does_not_consume_next_function(self):
        header = 'inline void MyMutex::CreateInstance() override {}\n'
        text = header + 'void unrelated() {\n    int other; // r3\n}\n'
        self.assertTrue(plan.is_fn_header(header.strip()))
        self.assertEqual(self.read_block(text), [(0, header.rstrip('\n'))])

    def test_nested_definition_stops_at_its_own_closing_brace(self):
        body = ('void valid() {\n'
                '    /* anonymous block */ {\n'
                '        inline void child() {}\n'
                '    }\n'
                '}\n')
        text = body + 'void unrelated() {}\n'
        self.assertEqual([line for _, line in self.read_block(text)], body.splitlines())

    def test_empty_function_at_nonzero_offset(self):
        prefix = 'void earlier() {\n}\n'
        header = 'void wanted() {}\n'
        text = prefix + header + 'void later() {}\n'
        self.assertEqual(self.read_block(text, len(prefix)), [(0, header.rstrip('\n'))])


if __name__ == '__main__':
    unittest.main()
