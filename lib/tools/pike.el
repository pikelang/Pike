;;; pike.el -- Font lock definitions for Pike and other LPC files.
;;; $Id: pike.el,v 1.23 2001/04/16 03:02:04 mast Exp $
;;; Copyright (C) 1995, 1996, 1997, 1998, 1999 Per Hedbor.
;;; This file is distributed as GPL

;;; Keywords: Pike, LPC, uLPC, µLPC, highlight

;;; To use:
;;;
;;; (require 'pike)
;;;
;;; Older Emacs versions doesn't come with a Pike-aware CC Mode (M-x
;;; c-version should report 5.23 or later), so you might have to
;;; upgrade that (see http://cc-mode.sourceforge.net/). You probably
;;; want this too in that case:
;;;
;;; (setq auto-mode-alist
;;;   (append '(("\\.pike$" . pike-mode)) auto-mode-alist)

(require 'font-lock)
(require 'custom)
(require 'cl)

;; Added in later font-lock versions. Copied here for backward
;; compatibility.
(defvar font-lock-preprocessor-face 'font-lock-keyword-face
  "Don't even think of using this.")
(defvar pike-font-lock-refdoc-face 'pike-font-lock-refdoc-face)
(defvar pike-font-lock-refdoc-init-face 'pike-font-lock-refdoc-init-face)
(defvar pike-font-lock-refdoc-init2-face 'pike-font-lock-refdoc-init2-face)
(defvar pike-font-lock-refdoc-keyword-face 'pike-font-lock-refdoc-keyword-face)
(defvar pike-font-lock-refdoc-error-face 'pike-font-lock-refdoc-error-face)

(defgroup pike-faces nil
  "Faces used by the pike color highlighting mode."
  :group 'font-lock
  :group 'faces)

(defface pike-font-lock-refdoc-face
  '((t))
  "Face to use for normal text in Pike documentation comments. It's
overlaid over the `font-lock-comment-face'."
  :group 'pike-faces)
(defface pike-font-lock-refdoc-init-face
  '((t (:bold t)))
  "Face to use for the magic init char of Pike documentation comments. It's
overlaid over the `font-lock-comment-face'."
  :group 'pike-faces)
(defface pike-font-lock-refdoc-init2-face
  '((t))
  "Face to use for the comment starters Pike documentation comments. It's
overlaid over the `font-lock-comment-face'."
  :group 'pike-faces)
(defface pike-font-lock-refdoc-keyword-face
  '((t))
  "Face to use for markup keywords Pike documentation comments. It's
overlaid over the `font-lock-reference-face'."
  :group 'pike-faces)
(defface pike-font-lock-refdoc-error-face
  '((((class color) (background light)) (:foreground "red"))
    (((class color)) (:foreground "hotpink"))
    (((background light)) (:foreground "white" :background "black"))
    (t (:foreground "black" :background "white")))
  "Face to use for invalid markup in Pike documentation comments."
  :group 'pike-faces)

(defconst pike-font-lock-type-regexp
  (concat "\\<\\("
	  "mixed\\|float\\|int\\|program\\|string\\|"
	  "function\\|array\\|mapping\\|multiset\\|object\\|void"
	  "\\)\\>")
  "Regexp for simple types. Must have exactly one submatch.")

(defconst pike-font-lock-keyword-regexp
  (concat "\\<\\("
	  "predef\\|import\\|default\\|case\\|class\\|break\\|continue\\|"
	  "do\\|else\\|for\\|if\\|return\\|switch\\|while\\|lambda\\|"
	  "catch\\|throw\\|foreach\\|inherit\\|typedef\\|constant"
	  "\\)\\>")
  "Regexp for keywords. Must have exactly one submatch.")

(defconst pike-font-lock-modifier-regexp
  (concat "\\<\\(public\\|inline\\|final\\|static\\|protected\\|"
	  "local\\|optional\\|private\\|nomask\\|variant\\)\\>")
  "Regexp for modifiers. Must have exactly one submatch.")

(defconst pike-font-lock-operator-identifiers
  (concat "`->=?\\|`\\+=?\\|`==\\|`\\[\\]=?\\|`()\\|`[!<>~]\\|"
	  "``?<<\\|``?>>\\|``?[%&*+/^|-]")
  "Regexp for operator identifiers. Must have zero submatches.")

; Problems: We really should allow all unicode characters...
(let ((capital-letter "A-Z\300-\326\330-\337")
      (letter "a-zA-Z\241-\377_")
      (digit  "0-9"))

  (defconst pike-font-lock-type-chars
    ;; Not using letter here since this string is used in
    ;; skip-chars-backward, which at least in Emacs 19.34 is broken
    ;; and hangs when it gets characters in the high eight bit range.
    (concat "a-zA-Z_" digit " \t\n\r:()[].|,"))

  (defconst pike-font-lock-identifier-regexp
    (concat "\\(\\<[" letter "][" letter digit "]*\\>\\|"
	    pike-font-lock-operator-identifiers
	    "\\)")
    "Regexp which should match all Pike identifiers.
Must have exactly one submatch.")

  (defconst pike-font-lock-class-name-regexp
    (concat "\\<\\([" capital-letter "][" letter digit "]*\\)\\>")
    "Regexp which should match a class name.
The name is assumed to begin with a capital letter.")

  (defconst pike-font-lock-identifier-or-integer
    ;; The identifier should get submatch 1 but not the integer.
    (concat pike-font-lock-identifier-regexp "\\|[" digit "]+")))

(defconst pike-font-lock-semantic-whitespace
  (concat "[ \t\n\r]*"
	  "\\("				; 1
	  (concat "\\("			; 2
		  "//[^\n]*"
		  "\\|"
		  "/\\*\\([^*]\\|\\*[^/]\\)*\\*/" ; 3
		  "\\)")
	  "[ \t\n\r]*"
	  "\\)*"))

(defconst pike-font-lock-qualified-identifier
  (concat "\\(\\.?"			; 1
	  pike-font-lock-identifier-regexp ; 2
	  "\\)+"))

(defun pike-font-lock-hack-file-coding-system-perhaps ( foo )
  (interactive)
  (message "charset %s" (buffer-substring (match-beginning 2) (match-end 2)))
  (condition-case fel
      (if (and (fboundp 'set-buffer-file-coding-system)
	       (fboundp 'symbol-concat))
	  (let ((coding (buffer-substring 
			 (match-beginning 2) (match-end 2))))
	    (set-buffer-file-coding-system (symbol-concat coding))))
    (t nil)))

(defconst pike-font-lock-maybe-type
  (concat pike-font-lock-identifier-regexp "[ \t\n\r]*" ; 1
	  "\\(\\.\\.\\.[ \t\n\r]*\\)?"	; 2
	  (concat "\\("			; 3
		  "[\]\)][\]\) \t\n\r]*"
		  pike-font-lock-semantic-whitespace ; 4-6
		  (concat "\\("		; 7
			  pike-font-lock-identifier-regexp ; 8
			  "\\|\("
			  "\\)")
		  "\\|"
		  pike-font-lock-semantic-whitespace ; 9-11
		  pike-font-lock-identifier-regexp ; 12
		  "\\)")))

(defvar pike-font-lock-last-type-end nil)
(defvar pike-font-lock-real-limit nil)

(defun pike-font-lock-find-first-type (limit)
  "Finds the first identifier/keyword in a type."
  ;; The trick we use here is (in principle) to find two consecutive
  ;; identifiers without an operator between them. It works only if
  ;; things like keywords have been fontified first.
  (let* ((start (point)) cast cast-end-pos no-face-pos
	 (large-pos (point-max)) (large-neg (- large-pos)))
    ;; We store the limit given to this function for use in the
    ;; anchored functions, since font-lock somewhat obnoxiously always
    ;; limits anchors to the same line, which we want to ignore.
    (setq pike-font-lock-real-limit limit)
    (catch 'done
      (while (re-search-forward pike-font-lock-maybe-type limit t)
	(if (not (memq (get-text-property (match-beginning 0) 'face)
		       '(font-lock-type-face nil)))
	    ;; Ignore a match where what we think is a type is already
	    ;; highlighted as a non-type.
	    (if (setq no-face-pos
		      (text-property-any (match-beginning 0) limit 'face nil))
		;; Jump past the highlighted region. This avoids bogus
		;; hits especially inside strings and comments.
		(goto-char no-face-pos)
	      (throw 'done nil))
	  (goto-char (setq pike-font-lock-last-type-end
			   (or (match-beginning 7) (match-beginning 12))))
	  (catch 'continue
	    (forward-comment large-neg)
	    (when (setq cast (assq (preceding-char) '((?\) . ?\() (?\] . ?\[))))
	      ;; This might be a cast.
	      (setq cast-end-pos (1- (point))
		    cast (cdr cast)))
	    ;; Now at the end of something that might be a type. Back
	    ;; up to the beginning of it by balanced sexps, accepting
	    ;; only identifiers and the type operators in between.
	    (condition-case nil
		(let ((type-min-pos
		       (save-excursion
			 ;; NB: Doesn't work if there are comments in the type.
			 (skip-chars-backward pike-font-lock-type-chars)
			 (point))))
		  (goto-char (scan-sexps (point) -1))
		  (if (< (point) type-min-pos) (throw 'continue nil))
		  (while (or (eq (following-char) ?\()
			     (progn
			       (forward-comment large-neg)
			       (when (memq (preceding-char) '(?| ?.))
				 (backward-char)
				 t)))
		    (goto-char (scan-sexps (point) -1))
		    (if (< (point) type-min-pos) (throw 'continue nil))
		    (setq cast nil)))
	      ;; Should only get here if the scan-sexps above fails.
	      (error nil))
	    (forward-comment large-pos)
	    (when (and (save-excursion
			 (beginning-of-line)
			 (looking-at "\\s *#\\s *define\\s *"))
		       (eq (match-end 0) (point)))
	      ;; Got to check for the case "#define x(foo) bar".
	      (throw 'continue nil))
	    (when (eq (following-char) cast)
	      ;; Jumped over exactly one sexp surrounded with ( ) or [
	      ;; ], so it's a cast and the type begins inside it. Also
	      ;; make sure that pike-font-lock-find-following-identifier
	      ;; doesn't highlight any following identifier.
	      (skip-chars-forward " \t\n\r([")
	      (setq pike-font-lock-last-type-end cast-end-pos))
	    (when (and (not (looking-at
"\\(if\\|while\\|for\\|foreach\\|switch\\|lambda\\)\\>[^_]"))
		       (looking-at pike-font-lock-identifier-regexp))
	      (when (< (point) start)
		;; Type started before the start of the search, so we
		;; jump to the first identifier in the type that's after
		;; the start. This search should never fail.
		(goto-char start)
		(re-search-forward pike-font-lock-identifier-or-integer limit))
	      (goto-char (match-end 0))
	      (throw 'done t)))
	  (goto-char pike-font-lock-last-type-end)
	  )))))

(defun pike-font-lock-find-next-type (limit)
  "Finds the next identifier/keyword in a type.
Used after `pike-font-lock-find-first-type' or
`pike-font-lock-match-next-type' have matched."
  (skip-chars-forward " \t\n\r:().|," pike-font-lock-last-type-end)
  (when (< (point) pike-font-lock-last-type-end)
    (if (looking-at pike-font-lock-identifier-or-integer)
	(goto-char (match-end 0))
      (goto-char pike-font-lock-last-type-end)
      nil)))

(defvar pike-font-lock-more-identifiers nil)

(defun pike-font-lock-find-following-identifier (limit)
  "Finds the following identifier after a type.
Used after `pike-font-lock-find-first-type',
`pike-font-lock-find-next-type' or
`pike-font-lock-find-following-identifier' have matched. Should the
variable name be followed by a comma after an optional value, we
reposition the cursor to fontify more identifiers."
  (when (looking-at pike-font-lock-identifier-regexp)
    (let ((match (match-data))
	  (start (point))
	  (value-start (match-end 1))
	  (large-pos (point-max))
	  (more pike-font-lock-more-identifiers)
	  chr)
      (goto-char value-start)
      (forward-comment large-pos)
      (setq chr (following-char)
	    pike-font-lock-more-identifiers nil)
      (prog1
	  (cond
	   ((and (eq chr ?\() (not more))
	    ;; It's a function identifier. Make the first submatch
	    ;; second to get the right face.
	    (setcdr (cdr match) (cons nil (cons nil (cdr (cdr match)))))
	    t)
	   ((eq chr ?,)
	    ;; It's a variable identifier in a list containing more
	    ;; identifiers.
	    (forward-char)
	    (forward-comment large-pos)
	    (if (> (point) pike-font-lock-real-limit)
		(goto-char pike-font-lock-real-limit))
	    ;; Signal that we've gone on looking at an identifier that
	    ;; isn't first in a list. It can't be a function then.
	    (setq pike-font-lock-more-identifiers t)
	    t)
	   ((memq chr '(?\; ?\)))
	    ;; It's a variable identifier at the end of a list.
	    t)
	   ((eq chr ?=)
	    ;; It's a variable identifier with a value assignment.
	    ;; Move over it to the next comma, if any.
	    (condition-case nil
		(save-restriction
		  (narrow-to-region (point) pike-font-lock-real-limit)
		  ;; Note: Both `scan-sexps' and the second goto-char can
		  ;; generate an error which is caught by the
		  ;; `condition-case' expression.
		  (while (progn
			   (forward-comment large-pos)
			   (not (looking-at "\\(\\(,\\)\\|;\\|$\\)")))
		    (goto-char (or (scan-sexps (point) 1) (point-max))))
		  (goto-char (match-end 2)) ; non-nil
		  (forward-comment large-pos))
	      (error
	       (goto-char value-start)))
	    t)
	   ((>= (point) pike-font-lock-real-limit)
	    (goto-char pike-font-lock-real-limit)
	    t)
	   (t
	    (if more (goto-char start))
	    nil))
	(set-match-data match)))))

(defun pike-font-lock-find-label (limit)
  (catch 'found
    (let ((large-neg (- (point-max))))
      (while (re-search-forward (concat pike-font-lock-identifier-regexp ":[^:]") limit t)
	(unless
	    ;; Ignore hits inside highlighted stuff.
	    (get-text-property (match-beginning 1) 'face)
	  (save-excursion
	    (goto-char (match-beginning 1))
	    (forward-comment large-neg)
	    (if (or
		 ;; Check for a char that precedes a statement.
		 (memq (preceding-char) '(?\} ?\{ ?\) ?\;))
		 ;; Check for a keyword that precedes a statement.
		 (condition-case nil
		     (progn (backward-sexp) nil)
		   (error t))
		 (save-match-data
		   (looking-at "\\(else\\|do\\)\\>[^_]")))
		(throw 'found t))))))))

(defconst pike-font-lock-some
  `(;; Keywords:
    (,(concat pike-font-lock-keyword-regexp "[^_]")
     1 font-lock-keyword-face)
	 
    ;; Modifiers:
    (,(concat pike-font-lock-modifier-regexp "[^_]")
     1 font-lock-preprocessor-face)

    ;; Class names:
    (,(concat "\\(\\<class\\>\\)\\s *"
	      pike-font-lock-identifier-regexp)
     (1 font-lock-keyword-face)
     (2 font-lock-function-name-face))

    ;; Labels in statements:
    (,(concat "\\<\\(break\\|continue\\)\\s *"
	      pike-font-lock-identifier-regexp)
     2 font-lock-reference-face)))

(defconst pike-font-lock-more
  (append `(("^#!.*$" 0 font-lock-comment-face)

	    ("^[ \t]*#[ \t]*error\\(.*\\)$"
	     (1 font-lock-string-face))

	    ;; #charset char-set-name
	    ;; Defines the file charset.
	    ("^[ \t]*\\(#[ \t]*charset\\)[ \t]*\\(.*\\)$"
	     (2 font-lock-keyword-face)
	     (pike-font-lock-hack-file-coding-system-perhaps))

	    ;; Fontify filenames in #include <...> as strings.
	    ("^[ \t]*#[ \t]*include[ \t]+\\(<[^>\"\n]+>\\)"
	     1 font-lock-string-face)

	    ("^[ \t]*#[ \t]*define[ \t]+\\(\\sw+\\)\("
	     1 font-lock-function-name-face)
	    ("^[ \t]*#[ \t]*define[ \t]+\\(\\sw+\\)"
	     1 font-lock-variable-name-face)

	    ;; Fontify symbol names in #if ...defined 
	    ;; etc preprocessor directives.
	    ("^[ \t]*#[ \t]*\\(el\\)?if\\>"
	     ("\\<\\(defined\\|efun\\|constant\\)\\>[ \t]*(?\\(\\sw+\\)?" 
	      nil nil
	      (1 font-lock-reference-face)
	      (2 font-lock-variable-name-face nil t)))

	    ("^[ \t]*\\(#[ \t]*[a-z]+\\)\\>[ \t]*\\(.*\\)?"
	     (1 font-lock-reference-face) 
	     (2 font-lock-variable-name-face nil t)))

	  pike-font-lock-some

	  `(;; Case labels with a qualified identifier.
	    (,(concat "\\<case\\>\\s *\\("
		      pike-font-lock-qualified-identifier
		      "\\):")
	     1 font-lock-reference-face)

	    ;; Scopes.
	    ((lambda (limit)
	       (when (re-search-forward
		      ,(concat pike-font-lock-identifier-regexp
			       "\\s *\\(\\.\\|::\\)\\s *\\(\\sw\\|`\\)")
		      limit t)
		 ;; Must back up the last bit since it can be the next
		 ;; identifier to match.
		 (goto-char (match-beginning 3))))
	     1 font-lock-reference-face))))

(defconst pike-font-lock-more-lastly
  `(;; Constants. This is after type font locking to get typed
    ;; constants correct (if and when they get valid).
    (,(concat "\\<constant[ \t\n\r]+"
	      pike-font-lock-identifier-regexp)
     1 font-lock-variable-name-face)

    ;; Simple types that might not have been catched by earlier rules.
    (,pike-font-lock-type-regexp
     1 font-lock-type-face)

    ;; Labels. Do this after the type font locking to avoid
    ;; highlighting "int" in "mapping(int:string)" as a label.
    (pike-font-lock-find-label
     1 font-lock-reference-face)))

(defconst pike-font-lock-keywords-1
  (append pike-font-lock-some

	  `(;; Case labels followed by anything.
	    (,(concat "\\<case\\>\\s *\\(.*\\):")
	     1 font-lock-reference-face)))
  "For consideration as a value of `pike-font-lock-keywords'.
This does fairly subdued highlighting.")

(defconst pike-font-lock-keywords-2
  (let ((decl-start (concat "\\("	; 1
			    pike-font-lock-type-regexp ; 2
			    "\\|"
			    "^\\s *"
			    (concat "\\(" ; 3
				    pike-font-lock-class-name-regexp ; 4
				    (concat "\\(\\." ; 5
					    pike-font-lock-identifier-regexp ; 6
					    "\\)*")
				    "\\)")
			    "\\)"
			    "[ \t\n\r]*")))
    (append pike-font-lock-more

	    `(;; Function declarations.
	      (,(concat decl-start
			pike-font-lock-identifier-regexp ; 7
			"[ \t\n\r]*(")
	       (3 font-lock-type-face nil t)
	       (7 font-lock-function-name-face))

	      ;; Variable declarations.
	      (,(concat decl-start
			pike-font-lock-identifier-regexp) ; 7
	       (3 font-lock-type-face nil t)
	       (7 font-lock-variable-name-face)))

	    pike-font-lock-more-lastly))
  "For consideration as a value of `pike-font-lock-keywords'.
This adds as much highlighting as possible that still not very complex
to calculate. Among other things it means that only simple types are
recognized.")

(defconst pike-font-lock-keywords-3
  (append `(;; Autodoc comments.
	    (,(concat "^\\([^/]\\|/[^/]\\)*"
		      "\\(//[^.!|\n\r]\\|\\(//\\)\\([.!|]\\)\\([^\n\r]*\\)\\)")
	     (3 pike-font-lock-refdoc-init2-face prepend t)
	     (4 pike-font-lock-refdoc-init-face prepend t)
	     (5 pike-font-lock-refdoc-face prepend t)
	     ("\\(@\\(\\w+{?\\|\\[[^\]]*\\]\\|[@}]\\|$\\)\\)\\|\\(@.\\)"
	      (if (match-end 4) (goto-char (match-end 4)) (end-of-line))
	      nil
	      (1 font-lock-reference-face t t)
	      (1 pike-font-lock-refdoc-keyword-face prepend t)
	      (3 pike-font-lock-refdoc-error-face t t))
	     ((lambda (limit)
		(if (looking-at "[ \t]*@\\(decl\\|member\\|index\\|elem\\)")
		    (progn
		      (put-text-property (match-end 0) limit 'face nil)
		      (goto-char limit)
		      t)))
	      (if (match-end 4) (goto-char (match-end 4)) (end-of-line))
	      nil)
	     ))

	  pike-font-lock-more

	  `(;; Catches most types and declarations.
	    (pike-font-lock-find-first-type
	     (1 font-lock-type-face nil t)
	     (pike-font-lock-find-next-type
	      nil nil
	      (1 font-lock-type-face nil t))
	     (pike-font-lock-find-following-identifier
	      nil nil
	      (1 font-lock-variable-name-face nil t)
	      (2 font-lock-function-name-face nil t))))

	  pike-font-lock-more-lastly)
  "For consideration as a value of `pike-font-lock-keywords'.
This adds highlighting that can be quite computationally intensive but
provides almost correct highlighting of e.g. types.")

(defvar pike-font-lock-keywords pike-font-lock-keywords-1
  "Additional expressions to highlight in Pike mode.")

;; XEmacs way.
(put 'pike-mode 'font-lock-defaults 
      '((pike-font-lock-keywords
     pike-font-lock-keywords-1 pike-font-lock-keywords-2
     pike-font-lock-keywords-3)
        nil nil ((?_ . "w")) beginning-of-defun
        (font-lock-mark-block-function . mark-defun)))

;; GNU Emacs way.
(if (and (boundp 'font-lock-defaults-alist)
	 (not (assq 'pike-mode font-lock-defaults-alist)))
    (setq font-lock-defaults-alist
	  (cons
	   (cons 'pike-mode
		 '((pike-font-lock-keywords pike-font-lock-keywords-1
		    pike-font-lock-keywords-2 pike-font-lock-keywords-3)
		   nil nil ((?_ . "w")) beginning-of-defun
		   (font-lock-mark-block-function . mark-defun)))
	   font-lock-defaults-alist)))

;; Autoload spec for older emacsen that doesn't come with a Pike aware
;; CC Mode. Doesn't do any harm in later emacsen.
(autoload 'pike-mode "cc-mode" "Major mode for editing Pike code.")

(provide 'pike)
