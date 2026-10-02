# q573 / native make entry

FreeBSD base makeの実機parse/変数・-j転送とGNU make no-config help/native entry dry-runを確認。GNUmakefileは従来Makefileを読む、FreeBSDのmakefileはGNU規則を独立native mkへ転送。CCと空白入りDESTDIRをquote、BSD MAKEFLAGSをGNUへ渡さない。全targetにzedBSD toolchain依存なし。

FreeBSD defaultPythonをpython3 packageのcommandへ変更し、Mesonのnative moduleと同じdefaultPythonを利用する。直接起動は既存--session（150秒のdevelopment deadline解除）を明記。Linux GDM登録は従来の別install-session。FreeBSD GDMはユーザー撤回、source/registrationなし。C/renderer/HAL/toolchain変更なし。git diff --check PASS。実build/installと全文最終レビューはp007で実行。
