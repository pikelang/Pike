;;; pike.el -- Major mode for editing Pike and other LPC files.
;;; $Id: pike.el,v 1.3 1999/06/27 15:37:10 mast Exp $
;;; Copyright (C) 1995, 1996, 1997, 1998, 1999 Per Hedbor.
;;; This file is distributed as GPL

;;; Keywords: Pike, LPC, uLPC, µLPC, highlight

;;; This file also provides a highlight-syntax table for Pike.
;;; In fact, it more or less require that you can set the highlight
;;; table.. It require Emacs 19.30 or later

;;; This file is modified by Martin Stjernholm <mast@lysator.liu.se>
;;; to work with CC Mode that have Pike support.

;;; Modified again by Per to support highlighting of 'new style' pike.
;;; All pike manual stuff removed, there are normal manpages nowdays.

;;; (load "pike")
;;; (setq auto-mode-alist 
;;;   (append '(("\\.pike$" . pike-mode)) auto-mode-alist)

;;; Known problems:
;;; The highlighting is at times quite bogus.

(require 'font-lock)

;; Added in later font-lock versions. Copied here for backward
;; compatibility.
(defvar font-lock-preprocessor-face 'font-lock-keyword-face
  "Don't even think of using this.")

(defconst pike-font-lock-keywords-1 nil
 "For consideration as a value of `pike-font-lock-keywords'.
This does fairly subdued highlighting.")

(defconst pike-font-lock-keywords-2 nil
 "For consideration as a value of `pike-font-lock-keywords'.
This adds highlighting of types and identifier names.")

(defconst pike-font-lock-keywords-3 nil
 "For consideration as a value of `pike-font-lock-keywords'.
This adds highlighting of Java documentation tags, such as @see.")

(defconst pike-font-lock-type-regexp
  (concat "\\<\\("
"mixed\\|"
"float\\|"
"int\\|"
"program\\|"
"string\\|"
"function\\|"
"function(.*)\\|"
"array\\|"
"array(.*)\\|"
"function\\|"
"function(.*)\\|"
"mapping\\|"
"mapping(.*)\\|"
"multiset\\|"
"multiset(.*)\\|"
"object\\|"
"object(.*)\\|"
"void\\|"
"constant\\|"
"class"
	"\\)\\>)*")
  "Regexp which should match a primitive type.")



; Problems: We really should allow 
(let ((capital-letter "A-Z\300-\326\330-\337")
      (letter "a-zA-Z\241-\377_")
      (digit  "0-9")
      )

  (defconst pike-font-lock-identifier-regexp
    (concat "\\<\\([" letter "][" letter digit "]*\\)\\>")
    "Regexp which should match all Pike identifiers.")

  (defconst pike-font-lock-class-name-regexp
    (concat "\\<\\([" capital-letter "][" letter digit "]*\\)\\>")
    "Regexp which should match a class name.
The name is assumed to begin with a capital letter.")


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

  (defconst pike-modifier-regexp
    (concat "\\<\\(public\\|inline\\|final\\|static\\|protected\\|"
	    "local\\|private\\|nomask\\|\\)\\>"))
  (defconst pike-operator-identifiers 
    (concat "``?\\(!=\\|->=?\\|<[<=]\\|==\\|>[=>]\\|\\[\\]=?"
	    "\\|[!%&+*/<>^|~-]\\)"))

    ;; Basic font-lock support:
  (setq pike-font-lock-keywords-1
	(list
	 ;; Keywords:
	 (list        
	  (concat
	   "\\<\\("
	   "predef\\|"
	   "import\\|"
	   "default\\|"
	   "case\\|"
	   "class\\|"
	   "break\\|"
	   "continue\\|"
	   "do\\|"
	   "else\\|"
	   "for\\|"
	   "if\\|"
	   "return\\|"
	   "switch\\|"
	   "while\\|"
	   "lambda\\|"
	   "catch\\|"
	   "throw\\|"
	   "foreach\\|"
	   "inherit"
	   "\\)\\>")
	  1 'font-lock-keyword-face)
	 
	 ;; Modifiers:
	 (list pike-modifier-regexp 1 font-lock-preprocessor-face)

	 ;; Class names:
	 (list (concat "\\(\\<class\\>\\)\\s *" 
		       pike-font-lock-identifier-regexp)
	       (list 1 'font-lock-keyword-face)
	       (list 2 'font-lock-function-name-face))
        
	 
	 ;; Methods:
	 (list (concat "\\(" 
		       pike-font-lock-type-regexp "\\|"
		       pike-font-lock-class-name-regexp
		       "\\)"
		       "\\s *\\(\\[\\s *\\]\\s *\\)*"
		       pike-font-lock-identifier-regexp "\\s *(")
	       5
	       'font-lock-function-name-face)

	 ;; Case statements:
	 ;; Any constant expression is allowed.
	 '("\\<case\\>\\s *\\(.*\\):" 1 font-lock-reference-face)))

    ;; Types and declared variable names:
    (setq pike-font-lock-keywords-2
	  (append 
	   (list
	    (list pike-operator-identifiers 
		  0 
		  font-lock-function-name-face)

	    '("^#!.*$" 0 font-lock-comment-face)

	    '("^\\(#[ \t]*error\\)\\(.*\\)$" 
	      (1 font-lock-reference-face) 
	      (2 font-lock-comment-face))

	    ;; #charset char-set-name

	    ;; Defines the file charset. 

	    '("^\\(#[ \t]*charset\\)[ \t]*\\(.*\\)$" 
	      (1 font-lock-reference-face) 
	      (2 font-lock-keyword-face)
	      (pike-font-lock-hack-file-coding-system-perhaps
	       ))

	    ;; Fontify filenames in #include <...> as strings.
	    '("^#[ \t]*include[ \t]+\\(<[^>\"\n]+>\\)" 
	      1 font-lock-string-face)

	    '("^#[ \t]*define[ \t]+\\(\\(\\sw+\\)(\\)" 
	      2 font-lock-function-name-face)
	    ;; Fontify symbol names in #if ...defined 
	    ;; etc preprocessor directives.
	    '("^#[ \t]*if\\>"
	      ("\\<\\(defined\\|efun\\|constant\\)\\>[ \t]*(?\\(\\sw+\\)?" 
	       nil nil
	       (1 font-lock-reference-face) 
	       (2 font-lock-variable-name-face nil t)))

	    '("^\\(#[ \t]*[a-z]+\\)\\>[ \t]*\\(\\.*\\)?"
	      (1 font-lock-reference-face) 
	      (2 font-lock-variable-name-face nil t))
	    )

	   pike-font-lock-keywords-1

	   (list
	    ;; primitive types, can't be confused with anything else.
	    (list pike-font-lock-type-regexp
		  '(1 font-lock-type-face)
		  '(font-lock-match-pike-declarations
		    (goto-char (match-end 0))
		    (goto-char (match-end 0))
		    (0 font-lock-variable-name-face)))

	    ;; Declarations, class types and capitalized variables:
	    ;;
	    ;; Declarations are easy to recognize.  Capitalized words
	    ;; followed by a closing parenthesis are treated as casts
	    ;; if they also are followed by an expression.
	    ;; Expressions beginning with a unary numerical operator,
	    ;; e.g. +, can't be cast to an object type.
	    (list (concat pike-font-lock-class-name-regexp
			  "\\s *\\(\\[\\s *\\]\\s *\\)*"
			  "\\(\\<\\|$\\|)\\s *\\([\(\"]\\|\\<\\)\\)")
		  '(1 (save-match-data
			(save-excursion
			  (goto-char
			   (match-beginning 3))
			  'font-lock-type-face)))
		  (list (concat "\\=" pike-font-lock-identifier-regexp
				"\\.")
			'(progn
			   (goto-char (match-beginning 0))
			   (while (or (= (preceding-char) ?.)
				      (= (char-syntax (preceding-char)) ?w))
			     (backward-char)))
			'(goto-char (match-end 0))
			'(1 font-lock-reference-face)
			'(0 nil))	; Workaround for bug in XEmacs.
		  '(font-lock-match-pike-declarations
		    (goto-char (match-end 1))
		    (goto-char (match-end 0))
		    (1 font-lock-variable-name-face))))))

  ;; Modifier keywords
  (setq pike-font-lock-keywords-3
	(append

	 '(
	   ;; Feature scoping:
	   ;; These must come first or the Modifiers from keywords-1 will
	   ;; catch them.  We don't want to use override fontification here
	   ;; because then these terms will be fontified within comments.
	   ("\\<public\\>"    0 font-lock-preprocessor-face)
	   ("\\<inline\\>"   0 font-lock-preprocessor-face)
	   ("\\<final\\>" 0 font-lock-preprocessor-face)
	   ("\\<static\\>" 0 font-lock-preprocessor-face)
	   ("\\<protected\\>" 0 font-lock-preprocessor-face)
	   ("\\<local\\>" 0 font-lock-preprocessor-face)
	   ("\\<private\\>"   0 font-lock-preprocessor-face)
	   ("\\<nomask\\>" 0 font-lock-preprocessor-face))
	 pike-font-lock-keywords-2
	 )))

(defvar pike-font-lock-keywords pike-font-lock-keywords-1
  "Additional expressions to highlight in Pike mode.")

;; Match and move over any declaration/definition item after
;; point.  Does not match items which look like a type declaration
;; (primitive types and class names, i.e. capitalized words.)
;; Should the variable name be followed by a comma, we reposition
;; the cursor to fontify more identifiers.
(defun font-lock-match-pike-declarations (limit)
  "Match and skip over variable definitions."
  (while (looking-at ")")
    (forward-char 1))
  (if (looking-at "\\s *\\(\\[\\s *\\]\\s *\\)*")
      (goto-char (match-end 0)))
  (and
   (looking-at pike-font-lock-identifier-regexp)
   (save-match-data
     (not (string-match pike-font-lock-type-regexp
			(buffer-substring (match-beginning 1)
					  (match-end 1)))))
   (save-match-data
     (save-excursion
       (goto-char (match-beginning 1))
       (not (looking-at
	     (concat pike-font-lock-class-name-regexp
		     "\\s *\\(\\[\\s *\\]\\s *\\)*\\<")))))
   (save-match-data
     (condition-case nil
	 (save-restriction
	   (narrow-to-region (point-min) limit)
	   (goto-char (match-end 0))
	   ;; Note: Both `scan-sexps' and the second goto-char can
	   ;; generate an error which is caught by the
	   ;; `condition-case' expression.
	   (while (not (looking-at "\\s *\\(\\(,\\)\\|;\\|$\\)"))
	     (goto-char (or (scan-sexps (point) 1) (point-max))))
	   (goto-char (match-end 2)))   ; non-nil
       (error t)))))

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

(autoload 'pike-mode "cc-mode" "Major mode for editing Pike code.")
