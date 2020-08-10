
#pragma once

const char g_schema_invalid[] ="{\
  \"$schema\": \"http://json-schema.org/draft-04/schema#\",\
  \"type\": \"object\",\
  \"properties\": {\
    \"cameras\": {\
      \"type\": \"array\",\
      \"items\": [\
        {\
          \"type\": \"object\",\
          \"properties\": {\
            \"device node test\": {\
              \"type\": \"string\"\
            },\
            \"device node\": {\
              \"type\": \"string\"\
            },\
            \"card type\": {\
              \"type\": \"string\"\
            },\
            \"formats\": {\
              \"type\": \"array\",\
              \"items\": [\
                {\
                  \"type\": \"object\",\
                  \"properties\": {\
                    \"index\": {\
                      \"type\": \"integer\"\
                    },\
                    \"type\": {\
                      \"type\": \"string\"\
                    },\
                    \"format\": {\
                      \"type\": \"string\"\
                    },\
                    \"name\": {\
                      \"type\": \"string\"\
                    },\
                    \"res rates\": {\
                      \"type\": \"array\",\
                      \"items\": [\
                        {\
                          \"type\": \"object\",\
                          \"properties\": {\
                            \"resolution\": {\
                              \"type\": \"string\"\
                            },\
                            \"rates\": {\
                              \"type\": \"array\",\
                              \"items\": [\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                }\
                              ]\
                            }\
                          },\
                          \"required\": [\
                            \"resolution\",\
                            \"rates\"\
                          ]\
                        },\
                        {\
                          \"type\": \"object\",\
                          \"properties\": {\
                            \"resolution\": {\
                              \"type\": \"string\"\
                            },\
                            \"rates\": {\
                              \"type\": \"array\",\
                              \"items\": [\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                }\
                              ]\
                            }\
                          },\
                          \"required\": [\
                            \"resolution\",\
                            \"rates\"\
                          ]\
                        },\
                        {\
                          \"type\": \"object\",\
                          \"properties\": {\
                            \"resolution\": {\
                              \"type\": \"string\"\
                            },\
                            \"rates\": {\
                              \"type\": \"array\",\
                              \"items\": [\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                }\
                              ]\
                            }\
                          },\
                          \"required\": [\
                            \"resolution\",\
                            \"rates\"\
                          ]\
                        },\
                        {\
                          \"type\": \"object\",\
                          \"properties\": {\
                            \"resolution\": {\
                              \"type\": \"object\"\
                            },\
                            \"rates\": {\
                              \"type\": \"array\",\
                              \"items\": {}\
                            }\
                          },\
                          \"required\": [\
                            \"resolution\",\
                            \"rates\"\
                          ]\
                        }\
                      ]\
                    }\
                  },\
                  \"required\": [\
                    \"index\",\
                    \"type\",\
                    \"format\",\
                    \"name\",\
                    \"res rates\"\
                  ]\
                }\
              ]\
            }\
          },\
          \"required\": [\
            \"device node test\",\
            \"device node\",\
            \"card type\",\
            \"formats\"\
          ]\
        },\
        {\
          \"type\": \"object\",\
          \"properties\": {\
            \"device node\": {\
              \"type\": \"string\"\
            },\
            \"card type\": {\
              \"type\": \"string\"\
            },\
            \"formats\": {\
              \"type\": \"array\",\
              \"items\": {}\
            }\
          },\
          \"required\": [\
            \"device node\",\
            \"card type\",\
            \"formats\"\
          ]\
        },\
        {\
          \"type\": \"object\",\
          \"properties\": {\
            \"device node\": {\
              \"type\": \"string\"\
            },\
            \"card type\": {\
              \"type\": \"string\"\
            },\
            \"formats\": {\
              \"type\": \"array\",\
              \"items\": [\
                {\
                  \"type\": \"object\",\
                  \"properties\": {\
                    \"index\": {\
                      \"type\": \"integer\"\
                    },\
                    \"type\": {\
                      \"type\": \"string\"\
                    },\
                    \"format\": {\
                      \"type\": \"string\"\
                    },\
                    \"name\": {\
                      \"type\": \"string\"\
                    },\
                    \"res rates\": {\
                      \"type\": \"array\",\
                      \"items\": [\
                        {\
                          \"type\": \"object\",\
                          \"properties\": {\
                            \"resolution\": {\
                              \"type\": \"string\"\
                            },\
                            \"rates\": {\
                              \"type\": \"array\",\
                              \"items\": [\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                }\
                              ]\
                            }\
                          },\
                          \"required\": [\
                            \"resolution\",\
                            \"rates\"\
                          ]\
                        },\
                        {\
                          \"type\": \"object\",\
                          \"properties\": {\
                            \"resolution\": {\
                              \"type\": \"string\"\
                            },\
                            \"rates\": {\
                              \"type\": \"array\",\
                              \"items\": [\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                }\
                              ]\
                            }\
                          },\
                          \"required\": [\
                            \"resolution\",\
                            \"rates\"\
                          ]\
                        },\
                        {\
                          \"type\": \"object\",\
                          \"properties\": {\
                            \"resolution\": {\
                              \"type\": \"string\"\
                            },\
                            \"rates\": {\
                              \"type\": \"array\",\
                              \"items\": [\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                },\
                                {\
                                  \"type\": \"string\"\
                                }\
                              ]\
                            }\
                          },\
                          \"required\": [\
                            \"resolution\",\
                            \"rates\"\
                          ]\
                        },\
                        {\
                          \"type\": \"object\",\
                          \"properties\": {\
                            \"resolution\": {\
                              \"type\": \"object\"\
                            },\
                            \"rates\": {\
                              \"type\": \"array\",\
                              \"items\": {}\
                            }\
                          },\
                          \"required\": [\
                            \"resolution\",\
                            \"rates\"\
                          ]\
                        }\
                      ]\
                    }\
                  },\
                  \"required\": [\
                    \"index\",\
                    \"type\",\
                    \"format\",\
                    \"name\",\
                    \"res rates\"\
                  ]\
                }\
              ]\
            }\
          },\
          \"required\": [\
            \"device node\",\
            \"card type\",\
            \"formats\"\
          ]\
        },\
        {\
          \"type\": \"object\",\
          \"properties\": {\
            \"device node\": {\
              \"type\": \"string\"\
            },\
            \"card type\": {\
              \"type\": \"string\"\
            },\
            \"formats\": {\
              \"type\": \"array\",\
              \"items\": {}\
            }\
          },\
          \"required\": [\
            \"device node\",\
            \"card type\",\
            \"formats\"\
          ]\
        }\
      ]\
    }\
  },\
  \"required\": [\
    \"cameras\"\
  ]\
}";