"""Check configure.py's mappings without configuring or compiling a version."""
import ast
import json
import os
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest


ROOT = Path(__file__).resolve().parents[1]
UNIT = 'main/Speed/Indep/SourceLists/zWorld2'
VERSIONS = ('SLES-53558-A124', 'SLUS-21351', 'EUROPEGERMILESTONE', 'SPEED_EXE_1_3')


def load_apply(version):
    names = {'OBJDIFF_SYMBOL_MAPPINGS', 'OBJDIFF_GOWE69_SYMBOL_MAPPINGS',
             'OBJDIFF_SHARED_SOURCES', 'OBJDIFF_SHARED_SOURCE_MAPPINGS'}
    tree = ast.parse((ROOT / 'configure.py').read_text(encoding='utf-8'))
    selected = [node for node in tree.body
                if (isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name)
                    and node.target.id in names)
                or (isinstance(node, ast.FunctionDef) and node.name == 'apply_objdiff_symbol_mappings')]
    assert len(selected) == len(names) + 1
    namespace = {'Dict': dict, 'Path': Path, 'json': json, 'config': SimpleNamespace(version=version),
                 # La resolucion de sufijos necesita los objetos compilados; aqui no toca nada.
                 'resolver_sufijos': lambda unit: False}
    exec(compile(ast.Module(body=selected, type_ignores=[]), 'configure.py', 'exec'), namespace)
    return namespace['apply_objdiff_symbol_mappings']


class ObjdiffMappingTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix='nfsmw-mapping-test-')
        self.previous_cwd = Path.cwd()
        os.chdir(self.directory.name)

    def tearDown(self):
        os.chdir(self.previous_cwd)
        self.directory.cleanup()

    def apply(self, version, mappings=None):
        unit = {'name': UNIT}
        if mappings is not None:
            unit['symbol_mappings'] = mappings
        Path('objdiff.json').write_text(json.dumps({'units': [unit]}), encoding='utf-8')
        load_apply(version)()
        return json.loads(Path('objdiff.json').read_text(encoding='utf-8'))['units'][0]

    def test_gc_adds_thunk_mapping_and_preserves_unrelated_mapping(self):
        result = self.apply('GOWE69', {'another_target': 'another_source'})
        self.assertEqual(result['symbol_mappings'],
                         {'__tcf_4': '__tcf_3', 'another_target': 'another_source'})

    def test_other_versions_remove_inherited_gc_mapping(self):
        for version in VERSIONS:
            with self.subTest(version=version):
                result = self.apply(version, {'__tcf_4': '__tcf_3', 'another_target': 'another_source'})
                self.assertEqual(result['symbol_mappings'], {'another_target': 'another_source'})

    def test_other_versions_drop_empty_mapping(self):
        for version in VERSIONS:
            with self.subTest(version=version):
                self.assertNotIn('symbol_mappings', self.apply(version, {'__tcf_4': '__tcf_3'}))

    def test_other_versions_preserve_different_manual_mapping(self):
        for version in VERSIONS:
            with self.subTest(version=version):
                result = self.apply(version, {'__tcf_4': 'different_source'})
                self.assertEqual(result['symbol_mappings'], {'__tcf_4': 'different_source'})

    def test_other_versions_do_not_add_gc_mapping(self):
        for version in VERSIONS:
            with self.subTest(version=version):
                self.assertNotIn('symbol_mappings', self.apply(version))


if __name__ == '__main__':
    unittest.main()
