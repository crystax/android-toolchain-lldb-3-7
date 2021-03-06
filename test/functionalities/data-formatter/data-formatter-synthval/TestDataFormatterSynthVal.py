"""
Test lldb data formatter subsystem.
"""

import os, time
import unittest2
import lldb
from lldbtest import *
import lldbutil

class DataFormatterSynthValueTestCase(TestBase):

    mydir = TestBase.compute_mydir(__file__)

    @skipUnlessDarwin
    @dsym_test
    def test_with_dsym_and_run_command(self):
        """Test using Python synthetic children provider to provide a value."""
        self.buildDsym()
        self.data_formatter_commands()

    @skipIfFreeBSD # llvm.org/pr20545 bogus output confuses buildbot parser
    @dwarf_test
    @expectedFailureLinux('llvm.org/pr19011', ['clang'])
    def test_with_dwarf_and_run_command(self):
        """Test using Python synthetic children provider to provide a value."""
        self.buildDwarf()
        self.data_formatter_commands()

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)
        # Find the line number to break at.
        self.line = line_number('main.cpp', 'break here')

    def data_formatter_commands(self):
        """Test using Python synthetic children provider to provide a value."""
        self.runCmd("file a.out", CURRENT_EXECUTABLE_SET)

        lldbutil.run_break_set_by_file_and_line (self, "main.cpp", self.line, num_expected_locations=1, loc_exact=True)

        self.runCmd("run", RUN_SUCCEEDED)

        # The stop reason of the thread should be breakpoint.
        self.expect("thread list", STOPPED_DUE_TO_BREAKPOINT,
            substrs = ['stopped',
                       'stop reason = breakpoint'])

        # This is the function to remove the custom formats in order to have a
        # clean slate for the next test case.
        def cleanup():
            self.runCmd('type format clear', check=False)
            self.runCmd('type summary clear', check=False)
            self.runCmd('type filter clear', check=False)
            self.runCmd('type synth clear', check=False)

        # Execute the cleanup function during test case tear down.
        self.addTearDownHook(cleanup)
        
        x = self.frame().FindVariable("x")
        x.SetPreferSyntheticValue(True)
        y = self.frame().FindVariable("y")
        y.SetPreferSyntheticValue(True)
        z = self.frame().FindVariable("z")
        z.SetPreferSyntheticValue(True)

        x_val = x.GetValueAsUnsigned
        y_val = y.GetValueAsUnsigned
        z_val = z.GetValueAsUnsigned
        
        if self.TraceOn():
            print "x_val = %s; y_val = %s; z_val = %s" % (x_val(),y_val(),z_val())

        self.assertFalse(x_val() == 3, "x == 3 before synthetics")
        self.assertFalse(y_val() == 4, "y == 4 before synthetics")
        self.assertFalse(z_val() == 7, "z == 7 before synthetics")

        # now set up the synth
        self.runCmd("script from myIntSynthProvider import *")
        self.runCmd("type synth add -l myIntSynthProvider myInt")
        
        if self.TraceOn():
            print "x_val = %s; y_val = %s; z_val = %s" % (x_val(),y_val(),z_val())
        
        self.assertTrue(x_val() == 3, "x != 3 after synthetics")
        self.assertTrue(y_val() == 4, "y != 4 after synthetics")
        self.assertTrue(z_val() == 7, "z != 7 after synthetics")
        
        self.expect("frame variable x", substrs=['3'])
        self.expect("frame variable x", substrs=['theValue = 3'], matching=False)
        
        # check that an aptly defined synthetic provider does not affect one-lining
        self.expect("expression struct S { myInt theInt{12}; }; S()", substrs = ['(theInt = 12)'])
        
        # check that we can use a synthetic value in a summary
        self.runCmd("type summary add hasAnInt -s ${var.theInt}")
        hi = self.frame().FindVariable("hi")
        self.assertEqual(hi.GetSummary(), "42")

if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()
