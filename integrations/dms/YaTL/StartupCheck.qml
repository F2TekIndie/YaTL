import QtQuick
import qs.Common

QtObject {
    function check(done) {
        Proc.runCommand("yatl.dependencyCheck", ["yatlctl", "--version"], function(output, exitCode) {
            done(exitCode === 0 ? null : {
                title: "yatlctl is unavailable",
                details: "Install YaTL and ensure yatlctl is on PATH."
            })
        })
    }
}
