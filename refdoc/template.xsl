<?xml version='1.0'?>

<!-- this template transforms the autodoc XML files to HTML -->
<!-- the results may be a lot short of pretty - but this is -->
<!-- supposed to be fixed before we release anything public -->

<xsl:stylesheet>
  <xsl:output method="html" />

  <xsl:param rxml:type="color" name="background" select="'white'"/>
  <xsl:param rxml:type="color" name="type" select="'#202020'"/>
  <xsl:param rxml:type="color" name="method-name" select="'blue'"/>
  <xsl:param rxml:type="color" name="param-name" select="'#8000F0'"/>
  <xsl:param rxml:type="color" name="var-name" select="'#F000F0'"/>
  <xsl:param rxml:type="color" name="constant-name" select="'#F000F0'"/>
  <xsl:param rxml:type="color" name="program-name" select="'#005080'"/>

  <xsl:param name="ref" select="'red'"/>
  <xsl:param name="literal" select="'green'"/>
  <xsl:template match="text()"></xsl:template>

  <xsl:template match="*">
    <xsl:text>TEMPLATE ERROR, NOT HANDLED: <xsl:value-of select="name()"/></xsl:text>
  </xsl:template>

  <xsl:template match="/module">
    <html><head><title>Pike Reference Manual</title></head>
    <body bgcolor="{$background}">
    <h1>Pike Reference Manual - BETA</h1>
    <p><date type="iso" date="date" /></p>
      <xsl:apply-templates select="docgroup"/>
      <xsl:apply-templates select="module"/>
      <xsl:apply-templates select="class"/>
    </body></html>
  </xsl:template>

  <xsl:template match="module[@name != '']">
    <dl><dt>
        <table width="100%" cellpadding="3" cellspacing="0" border="0"><tr>
        <td bgcolor="#EEEEEE">
           <font size="+3">&nbsp; MODULE <b><xsl:value-of select="@name"/></b></font>
        </td></tr></table><br />
      </dt><dd>
        <dl><xsl:apply-templates select="./doc"/></dl>
        <xsl:for-each select="docgroup">
          <xsl:apply-templates select="."/>
        </xsl:for-each>
        <xsl:for-each select="class">
          <xsl:apply-templates select="."/>
        </xsl:for-each>
        <xsl:for-each select="module">
          <xsl:apply-templates select="."/>
        </xsl:for-each>
      </dd>
    </dl>
  </xsl:template>

  <xsl:template match="class">
    <dl>
      <dt><table width="100%" cellpadding="3" cellspacing="0" border="0"><tr>
        <td bgcolor="#EEEEEE">
        <font size="+3">&nbsp; CLASS <b>
            <font color="{$program-name}">
              <xsl:for-each select="ancestor::*[@name != '']"
                ><xsl:value-of select="@name"
                />.</xsl:for-each><xsl:value-of select="@name"/>
        </font></b></font>
        </td></tr></table><br />
      </dt>
      <dd>
        <dl><xsl:apply-templates select="doc"/></dl>
        <xsl:if test="inherit">
          <h3>Inherits:</h3>
          <ul><xsl:apply-templates select="inherit"/></ul>
        </xsl:if>
        <xsl:apply-templates select="docgroup"/>
        <xsl:apply-templates select="class"/>
      </dd>
    </dl>
  </xsl:template>

  <xsl:template match="inherit">
    <li><xsl:value-of select="classname"/></li>
  </xsl:template>

  <!-- DOCGROUPS -->

  <xsl:template name="show-class-path">
    <xsl:if test="count(ancestor::*[@name != '']) > 0">
      <font color="{$program-name}">
        <xsl:for-each select="ancestor::*[@name != '']">
          <xsl:value-of select="@name"/>
          <xsl:if test="position() != last()">.</xsl:if>
        </xsl:for-each>
      </font>
    </xsl:if>
  </xsl:template>

  <xsl:template name="show-class-path-arrow">
    <xsl:if test="count(ancestor::*[@name != '']) > 0">
      <xsl:call-template name="show-class-path"/>
      <xsl:choose>
        <xsl:when test="ancestor::class">
          <xsl:text>-&gt;</xsl:text>
        </xsl:when>
        <xsl:when test="ancestor::module">
          <xsl:text>.</xsl:text>
        </xsl:when>
      </xsl:choose>
    </xsl:if>
  </xsl:template>

  <xsl:template match="docgroup">
    <hr clear="all" noshade="noshade" size="1"/>
    <dl>
      <dt>
        <xsl:choose>
          <xsl:when test="@homogen-type">
            <font face="Helvetica">
            <xsl:value-of select="translate(substring(@homogen-type,1,1),'cmvi','CMVI')"/>
            <xsl:value-of select="substring(@homogen-type,2)"/>
            </font>
            <xsl:choose>
              <xsl:when test="@homogen-name">
		<!-- FIXME: Should have the module path here. -->
                <font size="+1"><b><xsl:value-of select="concat(' ', concat(@belongs, @homogen-name))"/></b></font>
              </xsl:when>
              <xsl:when test="count(./*[name() != 'doc']) > 1">s
              </xsl:when>
            </xsl:choose>
          </xsl:when>
          <xsl:otherwise>syntax</xsl:otherwise>
        </xsl:choose>
      </dt>
      <dd><p><xsl:apply-templates select="*[name() != 'doc']"/></p></dd>
      <xsl:apply-templates select="doc"/>
    </dl>
  </xsl:template>

  <xsl:template match="constant">
    <xsl:if test="position() != 1"><br/></xsl:if>
    <tt>
      constant
      <xsl:call-template name="show-class-path-arrow"/> 
      <font color="{$constant-name}">
        <xsl:value-of select="@name"/>
      </font>
      <xsl:if test="typevalue">
        =
        <xsl:apply-templates select="typevalue/*" mode="type"/>
      </xsl:if>
    </tt>
  </xsl:template>

  <xsl:template match="variable">
    <xsl:if test="position() != 1"><br/></xsl:if>
    <tt>
      <xsl:apply-templates select="type/*" mode="type"/>
      <xsl:value-of select="' '"/>
      <xsl:call-template name="show-class-path-arrow"/> 
      <b><font color="{$var-name}"><xsl:value-of select="@name"/>
      </font></b>
    </tt>
  </xsl:template>

  <xsl:template match="method">
    <xsl:if test="position() != 1"><br/></xsl:if>
    <tt>
      <xsl:apply-templates select="returntype/*" mode="type"/>
      <xsl:value-of select="' '"/>
      <xsl:call-template name="show-class-path-arrow"/>
      <b><font color="{$method-name}"><xsl:value-of select="@name"/>
      </font>(</b>
        <xsl:for-each select="arguments/argument">
           <xsl:apply-templates select="."/>
           <xsl:if test="position() != last()">, </xsl:if>
        </xsl:for-each>
      <b>)</b></tt>
  </xsl:template>

  <xsl:template match="argument">
    <xsl:choose>
      <xsl:when test="value">
        <font color="{$literal}"><xsl:value-of select="value"/></font>
      </xsl:when>
      <xsl:when test="type and @name">
        <xsl:apply-templates select="type/*" mode="type"/>
        <xsl:value-of select="' '"/>
        <font color="{$param-name}"><xsl:value-of select="@name"/></font>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <!-- TYPES -->

  <xsl:template match="object" mode="type">
    <xsl:choose>
      <xsl:when test="text()">
        <font color="{$program-name}"><xsl:value-of select="."/></font>
      </xsl:when>
      <xsl:otherwise>
        <font color="{$type}">object</font>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="multiset" mode="type">
    <font color="{$type}">multiset</font>
    <xsl:if test="indextype"
    >(<xsl:apply-templates select="indextype/*" mode="type"
    />)</xsl:if>
  </xsl:template>

  <xsl:template match="array" mode="type">
    <font color="{$type}">array</font>
    <xsl:if test="valuetype"
    >(<xsl:apply-templates select="valuetype/*" mode="type"
    />)</xsl:if>
  </xsl:template>

  <xsl:template match="mapping" mode="type">
    <font color="{$type}">mapping</font>
    <xsl:if test="indextype and valuetype"
      >(<xsl:apply-templates select="indextype/*" mode="type"/>
       : <xsl:apply-templates select="valuetype/*" mode="type"
    />)</xsl:if>
  </xsl:template>

  <xsl:template match="function" mode="type">
    <font color="{$type}">function</font>
    <xsl:if test="argtype or returntype"
     >(<xsl:for-each select="argtype/*">
        <xsl:apply-templates select="." mode="type"/>
        <xsl:if test="position() != last()">, </xsl:if>
      </xsl:for-each>
      :
      <xsl:apply-templates select="returntype/*" mode="type"
    />)</xsl:if>
  </xsl:template>

  <xsl:template match="varargs"  mode="type">
    <xsl:apply-templates select="*" mode="type"/>
    <xsl:text> ... </xsl:text>
  </xsl:template>

  <xsl:template match="or" mode="type">
    <xsl:for-each select="./*">
      <xsl:apply-templates select="." mode="type"/>
      <xsl:if test="position() != last()">|</xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="string|void|program|mixed|float" mode="type">
    <font color="{$type}">
      <xsl:value-of select="name()"/>
    </font>
  </xsl:template>

  <xsl:template match="int" mode="type">
    <font color="{$type}">int</font>
      <xsl:if test="min|max">(<font color="{$literal}">
        <xsl:value-of select="min"/>..<xsl:value-of select="max"/>
      </font>)</xsl:if>
  </xsl:template>

  <xsl:template match="p" mode="type"></xsl:template>

  <!-- DOC -->

  <xsl:template match="doc">
    <xsl:if test="text">
      <dt><font face="Helvetica">Description</font></dt>
      <xsl:apply-templates select="text" mode="doc"/>
    </xsl:if>
    <xsl:apply-templates select="group" mode="doc"/>
  </xsl:template>

  <xsl:template match="group" mode="doc">
    <xsl:apply-templates select="*[name(.) != 'text']" mode="doc"/>
    <xsl:apply-templates select="text" mode="doc"/>
  </xsl:template>

  <xsl:template match="param" mode="doc">
    <dt><font face="Helvetica">Parameter <tt>
        <font color="{$param-name}"><xsl:value-of select="@name"/></font>
    </tt></font></dt><dd></dd>
  </xsl:template>

  <xsl:template match="seealso" mode="doc">
    <dt><font face="Helvetica">See also</font></dt>    
  </xsl:template>

  <xsl:template match="*" mode="doc">
    <xsl:if test="parent::*[name() = 'group']">
      <dt><font face="Helvetica">
	<xsl:value-of select="translate(substring(name(),1,1),'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>
        <xsl:value-of select="substring(name(),2)"/>
      </font></dt>
      <xsl:apply-templates mode="doc"/>
    </xsl:if>
  </xsl:template>

  <xsl:template match="text" mode="doc">
    <dd><font face="Helvetica"><xsl:apply-templates mode="text"/></font></dd>
  </xsl:template>

  <!-- TEXT -->

  <xsl:template match="dl" mode="text">
    <dl>
      <xsl:for-each select="group">
        <xsl:apply-templates mode="text"/>
      </xsl:for-each>
    </dl>
  </xsl:template>

  <xsl:template match="item" mode="text">
    <dt><xsl:value-of select="@name"/></dt>
  </xsl:template>

  <xsl:template match="text" mode="text">
    <dd><xsl:apply-templates mode="text"/></dd>
  </xsl:template>

  <xsl:template match="mapping" mode="text">
    <dl>
      <xsl:for-each select="group">
        <xsl:for-each select="member">
          <dt>
            <tt>
              <font color="{$literal}"><xsl:value-of select="index"/></font>
              : <xsl:apply-templates select="type" mode="type"/>
            </tt>
          </dt><dd></dd>
        </xsl:for-each>
        <dd>
          <xsl:apply-templates select="text" mode="text"/>
        </dd>
      </xsl:for-each>
    </dl>
  </xsl:template>

  <xsl:template match="array" mode="text">
    <dl>
      <xsl:for-each select="group">
        <xsl:for-each select="elem">
          <dt>
            <tt>
              <xsl:apply-templates select="type" mode="type"/>
              <xsl:text> </xsl:text>
              <font color="{$literal}"><xsl:value-of select="index"/></font>
            </tt>
          </dt><dd></dd>
        </xsl:for-each>
        <dd>
          <xsl:apply-templates select="text" mode="text"/>
        </dd>
      </xsl:for-each>
    </dl>
  </xsl:template>

  <xsl:template match="int" mode="text">
    <dl>
      <xsl:for-each select="group">
        <xsl:for-each select="value">
          <dt>
            <tt>
              <font color="{$literal}"><xsl:value-of select="value"/></font>
            </tt>
          </dt><dd></dd>
        </xsl:for-each>
        <dd>
          <xsl:apply-templates select="text" mode="text"/>
        </dd>
      </xsl:for-each>
    </dl>
  </xsl:template>

  <xsl:template match="mixed" mode="text">
    <xsl:text><tt><xsl:value-of select="@name"/></tt> can have any of the following types:</xsl:text>
    <dl>
      <xsl:for-each select="group">
        <dt><tt><xsl:apply-templates select="type" mode="type"/></tt></dt>
	<dd><xsl:apply-templates select="text" mode="text"/></dd>
      </xsl:for-each>
    </dl>
  </xsl:template>

  <xsl:template match="ref" mode="text">
      <font face="courier" size="-1"><xsl:value-of select="."/></font>
  </xsl:template>

  <xsl:template match="i|p|b|tt" mode="text">
    <xsl:copy select=".">
      <xsl:apply-templates mode="text"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="pre|code" mode="text">
    <font face="courier"><xsl:copy select=".">
      <xsl:apply-templates mode="text"/>
    </xsl:copy></font>
  </xsl:template>

  <xsl:template match="text()" mode="text">
    <xsl:copy-of select="."/>
  </xsl:template>

</xsl:stylesheet>
