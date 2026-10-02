<!-- awesome-plan project=zedbsd record=q580 -->

# q580: Linux Keilandで標準GTK4 baselineを実測

Status: finished
Attempt: q580-i01 / uncleared
Phase: [ws114-p001](../ws114/phase001/phase.md)
Approval: current user / 2026-10-02 GTK4をdesktop担当の次作業に指定、利用上限回復後の再起動許可
Executor: P9 generation2、canonical writer Q1

Debian 13.7 guestの標準GTK4 4.18.6でwindow/menu/dialog、同一client clipboard、通常FileDialog、maximize/fullscreen等を実測し、[19行機能表](../ws114/gtk4-compat-matrix.md)へ証拠またはskip理由を保存した。二重CSD/SSDとrestore寸法増加は未確定の観測で、bug ticketや修正の判断はしていない。

[Terminal result](../ws114/phase001/q580-result.md)のとおり、interactive move/resize、cross-client clipboard、wheel/touch、D&D/PRIMARY、renderer/scale/IME等が未測定でPhaseはuncleared。P9 final `8556185a3`をmain `ff0522115`へ統合。専用QEMU/SSHは停止し、hash済みoverlayをignored buildに保全した。p002採否やsource修正は始めず、残測定を新Queueへ選定する。
