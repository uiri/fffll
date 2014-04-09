(defvar fffll-mode-hook nil)
(defvar fffll-tab-width 4)

(add-to-list 'auto-mode-alist '("\\.ff\\'" . fffll-mode))

(defun fffll-mode-indent-line ()
  "Indent current line as fffll code"
  (interactive)
  (beginning-of-line)
  (if (bobp)
      (indent-line-to 0)
    (let ((not-indented t) cur-indent)
      (save-excursion
	(forward-line -1)
	(setq cur-indent (current-indentation))
	(if (looking-at "^[ \t]*[}]")
	    (setq cur-indent (+ cur-indent fffll-tab-width)))
	(while not-indented
	  (if (looking-at "[{]")
	      (setq cur-indent (+ cur-indent fffll-tab-width))
	    (if (looking-at "[}]")
		(setq cur-indent (- cur-indent fffll-tab-width))
	      (if (looking-at "\n")
		  (setq not-indented nil))))
	  (forward-char)))
      (if (looking-at "[ \t]*[}]")
	  (setq cur-indent (- cur-indent fffll-tab-width)))
      (if (< cur-indent 0)
	  (setq cur-indent 0))
      (if cur-indent
	  (indent-line-to cur-indent)
	(indent-line-to 0)))))

(defconst fffll-font-lock-keywords
  (list
   '("^\\(--\\)\\([^*].*\\)$" (1 font-lock-comment-delimiter-face) (2 font-lock-comment-face))
   '("[^*]\\(--\\)\\([^*].*\\)$" (1 font-lock-comment-delimiter-face) (2 font-lock-comment-face))
   '("_\\([a-zA-Z0-9]*\\)" . font-lock-constant-face)
   '("%.*$" . font-lock-keyword-face)
   '("%[a-zA-Z0-9]+" . font-lock-keyword-face)
   '("\\([a-zA-Z]\\([A-Za-z0-9]*\\)\\)(" 1 font-lock-function-name-face)
   '("set(\\([a-zA-Z]\\([a-zA-Z0-9.]*\\)\\)" 1 font-lock-variable-name-face)
  '("\\([a-zA-Z]\\([a-zA-Z0-9.]*\\)\\):" 1 font-lock-variable-name-face)))

;; (defvar fffll-mode-syntax-table
;;   (let ((st (make-syntax-table)))
;;   (modify-syntax-entry ?. "w" st)
;;   (modify-syntax-entry ?- "14" st)
;;   (modify-syntax-entry ?* "23" st)
;;   st))

(defun fffll-mode ()
  "Major mode for editting fffll files"
  (interactive)
  (kill-all-local-variables)
  (setq indent-tabs-mode nil)
  ;(set-syntax-table fffll-mode-syntax-table)
  (set (make-local-variable 'font-lock-defaults) '(fffll-font-lock-keywords))
  (set (make-local-variable 'indent-line-function) 'fffll-mode-indent-line)
  (setq major-mode 'fffll-mode)
  (setq mode-name "fffll")
  (run-hooks 'fffll-mode-hook))

(provide 'fffll)
