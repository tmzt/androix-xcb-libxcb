<?xml version="1.0" encoding="utf-8"?>
<xsl:transform xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:output method="text"/>

  <xsl:strip-space elements="*"/>

  <xsl:variable name="nl"><xsl:text>
</xsl:text></xsl:variable>

  <xsl:template match="enum/item">
    <xsl:value-of select="concat('XCB', /xcb/@extension-name, ../@name, @name, $nl)"/>
  </xsl:template>

  <xsl:template match="event|eventcopy|error|errorcopy">
    <xsl:value-of select="concat('XCB', /xcb/@extension-name, @name, $nl)"/>
  </xsl:template>

  <xsl:template match="text()"/>

</xsl:transform>
