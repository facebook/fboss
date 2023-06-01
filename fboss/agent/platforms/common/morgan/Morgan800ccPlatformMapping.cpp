/*
 *  Copyright (c) 2023-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/morgan/Morgan800ccPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
  "ports": {
    "1": {
        "mapping": {
          "id": 1,
          "name": "eth1/1/1",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 8
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 9
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 10
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 11
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 12
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 13
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 7
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 14
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 15
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
            "18": {
                "pins": {
                  "iphy": [
                    {
                      "id": {
                        "chip": "IFG10",
                        "lane": 8
                      },
                      "tx": {
                        "pre": 0,
                        "pre2": 0,
                        "main": 765,
                        "post": -155,
                        "post2": 0,
                        "post3": 0
                      },
                      "rx": {
                        "ctlCode": 13,
                        "dspMode": 2,
                        "afeTrim": 16,
                        "acCouplingBypass": 1
                      }
                    },
                    {
                      "id": {
                        "chip": "IFG10",
                        "lane": 9
                      },
                      "tx": {
                        "pre": 0,
                        "pre2": 0,
                        "main": 765,
                        "post": -155,
                        "post2": 0,
                        "post3": 0
                      },
                      "rx": {
                        "ctlCode": 13,
                        "dspMode": 2,
                        "afeTrim": 16,
                        "acCouplingBypass": 1
                      }
                    },
                    {
                      "id": {
                        "chip": "IFG10",
                        "lane": 10
                      },
                      "tx": {
                        "pre": 0,
                        "pre2": 0,
                        "main": 765,
                        "post": -155,
                        "post2": 0,
                        "post3": 0
                      },
                      "rx": {
                        "ctlCode": 13,
                        "dspMode": 2,
                        "afeTrim": 16,
                        "acCouplingBypass": 1
                      }
                    },
                    {
                      "id": {
                        "chip": "IFG10",
                        "lane": 11
                      },
                      "tx": {
                        "pre": 0,
                        "pre2": 0,
                        "main": 765,
                        "post": -155,
                        "post2": 0,
                        "post3": 0
                      },
                      "rx": {
                        "ctlCode": 13,
                        "dspMode": 2,
                        "afeTrim": 16,
                        "acCouplingBypass": 1
                      }
                    }
                  ],
                  "transceiver": [
                    {
                      "id": {
                        "chip": "eth1/1",
                        "lane": 1
                      }
                    },
                    {
                      "id": {
                        "chip": "eth1/1",
                        "lane": 0
                      }
                    },
                    {
                      "id": {
                        "chip": "eth1/1",
                        "lane": 3
                      }
                    },
                    {
                      "id": {
                        "chip": "eth1/1",
                        "lane": 2
                      }
                    }
                  ]
                }
            }
      }
    },
    "9": {
        "mapping": {
          "id": 9,
          "name": "eth1/2/1",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 16
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 17
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 18
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 19
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 20
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 21
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 22
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 23
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
            "18": {
                "pins": {
                  "iphy": [
                    {
                      "id": {
                        "chip": "IFG10",
                        "lane": 16
                      },
                      "tx": {
                        "pre": 0,
                        "pre2": 0,
                        "main": 765,
                        "post": -155,
                        "post2": 0,
                        "post3": 0
                      },
                      "rx": {
                        "ctlCode": 13,
                        "dspMode": 2,
                        "afeTrim": 16,
                        "acCouplingBypass": 1
                      }
                    },
                    {
                      "id": {
                        "chip": "IFG10",
                        "lane": 17
                      },
                      "tx": {
                        "pre": 0,
                        "pre2": 0,
                        "main": 765,
                        "post": -155,
                        "post2": 0,
                        "post3": 0
                      },
                      "rx": {
                        "ctlCode": 13,
                        "dspMode": 2,
                        "afeTrim": 16,
                        "acCouplingBypass": 1
                      }
                    },
                    {
                      "id": {
                        "chip": "IFG10",
                        "lane": 18
                      },
                      "tx": {
                        "pre": 0,
                        "pre2": 0,
                        "main": 765,
                        "post": -155,
                        "post2": 0,
                        "post3": 0
                      },
                      "rx": {
                        "ctlCode": 13,
                        "dspMode": 2,
                        "afeTrim": 16,
                        "acCouplingBypass": 1
                      }
                    },
                    {
                      "id": {
                        "chip": "IFG10",
                        "lane": 19
                      },
                      "tx": {
                        "pre": 0,
                        "pre2": 0,
                        "main": 765,
                        "post": -155,
                        "post2": 0,
                        "post3": 0
                      },
                      "rx": {
                        "ctlCode": 13,
                        "dspMode": 2,
                        "afeTrim": 16,
                        "acCouplingBypass": 1
                      }
                    }
                  ],
                  "transceiver": [
                    {
                      "id": {
                        "chip": "eth1/2",
                        "lane": 0
                      }
                    },
                    {
                      "id": {
                        "chip": "eth1/2",
                        "lane": 1
                      }
                    },
                    {
                      "id": {
                        "chip": "eth1/2",
                        "lane": 2
                      }
                    },
                    {
                      "id": {
                        "chip": "eth1/2",
                        "lane": 3
                      }
                    }
                  ]
                }
            }
        }
    },
    "17": {
      "mapping": {
        "id": 17,
        "name": "eth1/3/1",
        "controllingPort": 17,
        "pins": [
          {
            "a": {
              "chip": "IFG9",
              "lane": 8
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "IFG9",
              "lane": 9
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "IFG9",
              "lane": 10
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "IFG9",
              "lane": 11
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "IFG9",
              "lane": 12
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "IFG9",
              "lane": 13
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "IFG9",
              "lane": 14
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "IFG9",
              "lane": 15
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0
      },
      "supportedProfiles": {
        "18": {
            "pins": {
              "iphy": [
                {
                  "id": {
                    "chip": "IFG9",
                    "lane": 8
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG9",
                    "lane": 9
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG9",
                    "lane": 10
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG9",
                    "lane": 11
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                }
              ],
              "transceiver": [
                {
                  "id": {
                    "chip": "eth1/3",
                    "lane": 0
                  }
                },
                {
                  "id": {
                    "chip": "eth1/3",
                    "lane": 1
                  }
                },
                {
                  "id": {
                    "chip": "eth1/3",
                    "lane": 2
                  }
                },
                {
                  "id": {
                    "chip": "eth1/3",
                    "lane": 3
                  }
                }
              ]
            }
        }
      }
    },
    "25": {
      "mapping": {
        "id": 25,
        "name": "eth1/4/1",
        "controllingPort": 25,
        "pins": [
          {
            "a": {
              "chip": "IFG8",
              "lane": 8
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "IFG8",
              "lane": 9
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "IFG8",
              "lane": 10
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "IFG8",
              "lane": 11
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 7
              }
            }
          },
          {
            "a": {
              "chip": "IFG8",
              "lane": 12
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "IFG8",
              "lane": 13
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "IFG8",
              "lane": 14
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "IFG8",
              "lane": 15
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 1
              }
            }
          }
        ],
        "portType": 0
      },
      "supportedProfiles": {
        "18": {
            "pins": {
              "iphy": [
                {
                  "id": {
                    "chip": "IFG8",
                    "lane": 12
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG8",
                    "lane": 13
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG8",
                    "lane": 14
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG8",
                    "lane": 15
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                }
              ],
              "transceiver": [
                {
                  "id": {
                    "chip": "eth1/4",
                    "lane": 2
                  }
                },
                {
                  "id": {
                    "chip": "eth1/4",
                    "lane": 3
                  }
                },
                {
                  "id": {
                    "chip": "eth1/4",
                    "lane": 0
                  }
                },
                {
                  "id": {
                    "chip": "eth1/4",
                    "lane": 1
                  }
                }
              ]
            }
        }
      }
    },
    "33": {
      "mapping": {
        "id": 33,
        "name": "eth1/5/1",
        "controllingPort": 33,
        "pins": [
          {
            "a": {
              "chip": "IFG11",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "IFG11",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "IFG11",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "IFG11",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "IFG11",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "IFG11",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "IFG11",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "IFG11",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0
      },
      "supportedProfiles": {
        "18": {
            "pins": {
              "iphy": [
                {
                  "id": {
                    "chip": "IFG11",
                    "lane": 0
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG11",
                    "lane": 1
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG11",
                    "lane": 2
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG11",
                    "lane": 3
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                }
              ],
              "transceiver": [
                {
                  "id": {
                    "chip": "eth1/5",
                    "lane": 0
                  }
                },
                {
                  "id": {
                    "chip": "eth1/5",
                    "lane": 1
                  }
                },
                {
                  "id": {
                    "chip": "eth1/5",
                    "lane": 2
                  }
                },
                {
                  "id": {
                    "chip": "eth1/5",
                    "lane": 3
                  }
                }
              ]
            }
        }
      }
    },
    "41": {
        "mapping": {
          "id": 41,
          "name": "eth1/6/1",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "IFG11",
                "lane": 16
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 17
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 18
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 7
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 19
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 20
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 21
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 22
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 23
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "18": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 20
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 21
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 22
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG11",
                      "lane": 23
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "49": {
        "mapping": {
          "id": 49,
          "name": "eth1/7/1",
          "controllingPort": 49,
          "pins": [
            {
              "a": {
                "chip": "IFG7",
                "lane": 8
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 9
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 10
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 11
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 12
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 13
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 14
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 15
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "18": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 8
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 9
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 10
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG7",
                      "lane": 11
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "57": {
        "mapping": {
          "id": 57,
          "name": "eth1/8/1",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "IFG6",
                "lane": 8
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 9
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 10
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 11
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 12
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 13
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 14
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 15
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "18": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 8
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 9
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 10
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG6",
                      "lane": 11
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },    
    "65": {
      "mapping": {
        "id": 65,
        "name": "eth1/9/1",
        "controllingPort": 65,
        "pins": [
          {
            "a": {
              "chip": "IFG5",
              "lane": 16
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "IFG5",
              "lane": 17
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 7
              }
            }
          },
          {
            "a": {
              "chip": "IFG5",
              "lane": 18
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "IFG5",
              "lane": 19
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "IFG5",
              "lane": 20
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "IFG5",
              "lane": 21
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "IFG5",
              "lane": 22
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "IFG5",
              "lane": 23
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 1
              }
            }
          }
        ],
        "portType": 0
      },
      "supportedProfiles": {
        "18": {
            "pins": {
              "iphy": [
                {
                  "id": {
                    "chip": "IFG5",
                    "lane": 20
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG5",
                    "lane": 21
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG5",
                    "lane": 22
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG5",
                    "lane": 23
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                }
              ],
              "transceiver": [
                {
                  "id": {
                    "chip": "eth1/9",
                    "lane": 2
                  }
                },
                {
                  "id": {
                    "chip": "eth1/9",
                    "lane": 3
                  }
                },
                {
                  "id": {
                    "chip": "eth1/9",
                    "lane": 0
                  }
                },
                {
                  "id": {
                    "chip": "eth1/9",
                    "lane": 1
                  }
                }
              ]
            }
        }
      }
    },
    "73": {
      "mapping": {
        "id": 73,
        "name": "eth1/10/1",
        "controllingPort": 73,
        "pins": [
          {
            "a": {
              "chip": "IFG5",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 7
              }
            }
          },
          {
            "a": {
              "chip": "IFG5",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "IFG5",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "IFG5",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "IFG5",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "IFG5",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "IFG5",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "IFG5",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 1
              }
            }
          }
        ],
        "portType": 0
      },
      "supportedProfiles": {
        "18": {
            "pins": {
              "iphy": [
                {
                  "id": {
                    "chip": "IFG5",
                    "lane": 4
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG5",
                    "lane": 5
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG5",
                    "lane": 6
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG5",
                    "lane": 7
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                }
              ],
              "transceiver": [
                {
                  "id": {
                    "chip": "eth1/10",
                    "lane": 2
                  }
                },
                {
                  "id": {
                    "chip": "eth1/10",
                    "lane": 3
                  }
                },
                {
                  "id": {
                    "chip": "eth1/10",
                    "lane": 0
                  }
                },
                {
                  "id": {
                    "chip": "eth1/10",
                    "lane": 1
                  }
                }
              ]
            }
        }
      }
    },
    "81": {
      "mapping": {
        "id": 81,
        "name": "eth1/11/1",
        "controllingPort": 81,
        "pins": [
          {
            "a": {
              "chip": "IFG0",
              "lane": 16
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "IFG0",
              "lane": 17
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 7
              }
            }
          },
          {
            "a": {
              "chip": "IFG0",
              "lane": 18
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "IFG0",
              "lane": 19
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "IFG0",
              "lane": 20
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "IFG0",
              "lane": 21
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "IFG0",
              "lane": 22
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "IFG0",
              "lane": 23
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 1
              }
            }
          }
        ],
        "portType": 0
      },
      "supportedProfiles": {
        "18": {
            "pins": {
              "iphy": [
                {
                  "id": {
                    "chip": "IFG0",
                    "lane": 20
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG0",
                    "lane": 21
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG0",
                    "lane": 22
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG0",
                    "lane": 23
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                }
              ],
              "transceiver": [
                {
                  "id": {
                    "chip": "eth1/11",
                    "lane": 3
                  }
                },
                {
                  "id": {
                    "chip": "eth1/11",
                    "lane": 2
                  }
                },
                {
                  "id": {
                    "chip": "eth1/11",
                    "lane": 0
                  }
                },
                {
                  "id": {
                    "chip": "eth1/11",
                    "lane": 1
                  }
                }
              ]
            }
        }
      }
    },
    "89": {
      "mapping": {
        "id": 89,
        "name": "eth1/12/1",
        "controllingPort": 89,
        "pins": [
          {
            "a": {
              "chip": "IFG0",
              "lane": 8
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "IFG0",
              "lane": 9
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 7
              }
            }
          },
          {
            "a": {
              "chip": "IFG0",
              "lane": 10
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "IFG0",
              "lane": 11
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "IFG0",
              "lane": 12
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "IFG0",
              "lane": 13
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "IFG0",
              "lane": 14
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "IFG0",
              "lane": 15
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 1
              }
            }
          }
        ],
        "portType": 0
      },
      "supportedProfiles": {
        "18": {
            "pins": {
              "iphy": [
                {
                  "id": {
                    "chip": "IFG0",
                    "lane": 12
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG0",
                    "lane": 13
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG0",
                    "lane": 14
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG0",
                    "lane": 15
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                }
              ],
              "transceiver": [
                {
                  "id": {
                    "chip": "eth1/12",
                    "lane": 2
                  }
                },
                {
                  "id": {
                    "chip": "eth1/12",
                    "lane": 3
                  }
                },
                {
                  "id": {
                    "chip": "eth1/12",
                    "lane": 0
                  }
                },
                {
                  "id": {
                    "chip": "eth1/12",
                    "lane": 1
                  }
                }
              ]
            }
        }
      }
    },
    "97": {
        "mapping": {
          "id": 97,
          "name": "eth1/13/1",
          "controllingPort": 97,
          "pins": [
            {
              "a": {
                "chip": "IFG3",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "18": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "105": {
        "mapping": {
          "id": 105,
          "name": "eth1/14/1",
          "controllingPort": 105,
          "pins": [
            {
              "a": {
                "chip": "IFG2",
                "lane": 16
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 7
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 17
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 18
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 19
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 20
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 21
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 22
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 23
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "18": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 20
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 21
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 22
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG2",
                      "lane": 23
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "113": {
      "mapping": {
        "id": 113,
        "name": "eth1/15/1",
        "controllingPort": 113,
        "pins": [
          {
            "a": {
              "chip": "IFG1",
              "lane": 16
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 7
              }
            }
          },
          {
            "a": {
              "chip": "IFG1",
              "lane": 17
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "IFG1",
              "lane": 18
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "IFG1",
              "lane": 19
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "IFG1",
              "lane": 20
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "IFG1",
              "lane": 21
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "IFG1",
              "lane": 22
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "IFG1",
              "lane": 23
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 3
              }
            }
          }
        ],
        "portType": 0
      },
      "supportedProfiles": {
        "18": {
            "pins": {
              "iphy": [
                {
                  "id": {
                    "chip": "IFG1",
                    "lane": 20
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG1",
                    "lane": 21
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG1",
                    "lane": 22
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                },
                {
                  "id": {
                    "chip": "IFG1",
                    "lane": 23
                  },
                  "tx": {
                    "pre": 0,
                    "pre2": 0,
                    "main": 765,
                    "post": -155,
                    "post2": 0,
                    "post3": 0
                  },
                  "rx": {
                    "ctlCode": 13,
                    "dspMode": 2,
                    "afeTrim": 16,
                    "acCouplingBypass": 1
                  }
                }
              ],
              "transceiver": [
                {
                  "id": {
                    "chip": "eth1/15",
                    "lane": 1
                  }
                },
                {
                  "id": {
                    "chip": "eth1/15",
                    "lane": 0
                  }
                },
                {
                  "id": {
                    "chip": "eth1/15",
                    "lane": 2
                  }
                },
                {
                  "id": {
                    "chip": "eth1/15",
                    "lane": 3
                  }
                }
              ]
            }
        }
      }
    },
    "121": {
        "mapping": {
          "id": 121,
          "name": "eth1/16/1",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "IFG1",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 7
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "18": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "IFG1",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 765,
                      "post": -155,
                      "post2": 0,
                      "post3": 0
                    },
                    "rx": {
                      "ctlCode": 13,
                      "dspMode": 2,
                      "afeTrim": 16,
                      "acCouplingBypass": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    }
  },
  "chips": [
    {
      "name": "IFG0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "IFG1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "IFG2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "IFG3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "IFG4",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "IFG5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "IFG6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "IFG7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "IFG8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "IFG9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "IFG10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "IFG11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "eth1/1",
      "type": 3,
      "physicalID": 0
    },
    {
      "name": "eth1/2",
      "type": 3,
      "physicalID": 1
    },
    {
      "name": "eth1/3",
      "type": 3,
      "physicalID": 2
    },
    {
      "name": "eth1/4",
      "type": 3,
      "physicalID": 3
    },
    {
      "name": "eth1/5",
      "type": 3,
      "physicalID": 4
    },
    {
      "name": "eth1/6",
      "type": 3,
      "physicalID": 5
    },
    {
      "name": "eth1/7",
      "type": 3,
      "physicalID": 6
    },
    {
      "name": "eth1/8",
      "type": 3,
      "physicalID": 7
    },
    {
      "name": "eth1/9",
      "type": 3,
      "physicalID": 8
    },
    {
      "name": "eth1/10",
      "type": 3,
      "physicalID": 9
    },
    {
      "name": "eth1/11",
      "type": 3,
      "physicalID": 10
    },
    {
      "name": "eth1/12",
      "type": 3,
      "physicalID": 11
    },
    {
      "name": "eth1/13",
      "type": 3,
      "physicalID": 12
    },
    {
      "name": "eth1/14",
      "type": 3,
      "physicalID": 13
    },
    {
      "name": "eth1/15",
      "type": 3,
      "physicalID": 14
    },
    {
      "name": "eth1/16",
      "type": 3,
      "physicalID": 15
    },
    {
      "name": "eth1/17",
      "type": 3,
      "physicalID": 16
    },
    {
      "name": "eth1/18",
      "type": 3,
      "physicalID": 17
    },
    {
      "name": "eth1/19",
      "type": 3,
      "physicalID": 18
    },
    {
      "name": "eth1/20",
      "type": 3,
      "physicalID": 19
    },
    {
      "name": "eth1/21",
      "type": 3,
      "physicalID": 20
    },
    {
      "name": "eth1/22",
      "type": 3,
      "physicalID": 21
    },
    {
      "name": "eth1/23",
      "type": 3,
      "physicalID": 22
    },
    {
      "name": "eth1/24",
      "type": 3,
      "physicalID": 23
    },
    {
      "name": "eth1/25",
      "type": 3,
      "physicalID": 24
    },
    {
      "name": "eth1/26",
      "type": 3,
      "physicalID": 25
    },
    {
      "name": "eth1/27",
      "type": 3,
      "physicalID": 26
    },
    {
      "name": "eth1/28",
      "type": 3,
      "physicalID": 27
    },
    {
      "name": "eth1/29",
      "type": 3,
      "physicalID": 28
    },
    {
      "name": "eth1/30",
      "type": 3,
      "physicalID": 29
    },
    {
      "name": "eth1/31",
      "type": 3,
      "physicalID": 30
    },
    {
      "name": "eth1/32",
      "type": 3,
      "physicalID": 31
    },
    {
      "name": "eth1/33",
      "type": 3,
      "physicalID": 32
    },
    {
      "name": "eth1/34",
      "type": 3,
      "physicalID": 33
    },
    {
      "name": "eth1/35",
      "type": 3,
      "physicalID": 34
    },
    {
      "name": "eth1/36",
      "type": 3,
      "physicalID": 35
    },
    {
      "name": "eth1/37",
      "type": 3,
      "physicalID": 36
    },
    {
      "name": "eth1/38",
      "type": 3,
      "physicalID": 37
    },
    {
      "name": "eth1/39",
      "type": 3,
      "physicalID": 38
    },
    {
      "name": "eth1/40",
      "type": 3,
      "physicalID": 39
    },
    {
      "name": "eth1/41",
      "type": 3,
      "physicalID": 40
    },
    {
      "name": "eth1/42",
      "type": 3,
      "physicalID": 41
    },
    {
      "name": "eth1/43",
      "type": 3,
      "physicalID": 42
    },
    {
      "name": "eth1/44",
      "type": 3,
      "physicalID": 43
    },
    {
      "name": "eth1/45",
      "type": 3,
      "physicalID": 44
    },
    {
      "name": "eth1/46",
      "type": 3,
      "physicalID": 45
    },
    {
      "name": "eth1/47",
      "type": 3,
      "physicalID": 46
    },
    {
      "name": "eth1/48",
      "type": 3,
      "physicalID": 47
    }
  ],
  "platformSettings": {
    "1": "/dev/uio0"
  },
  "platformSupportedProfiles": [
    {
      "factor": {
        "profileID": 11
      },
      "profile": {
        "speed": 10000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "medium": 1,
          "interfaceMode": 10,
          "interfaceType": 10
        }
      }
    },
    {
      "factor": {
        "profileID": 14
      },
      "profile": {
        "speed": 25000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 10,
          "interfaceType": 10
        }
      }
    },
    {
        "factor": {
          "profileID": 15
        },
        "profile": {
          "speed": 25000,
          "iphy": {
            "numLanes": 1,
            "modulation": 1,
            "fec": 74,
            "medium": 1,
            "interfaceMode": 10,
            "interfaceType": 10
          }
        }
    },
    {
        "factor": {
          "profileID": 18
        },
        "profile": {
          "speed": 40000,
          "iphy": {
            "numLanes": 4,
            "modulation": 1,
            "fec": 1,
            "medium": 2,
            "interfaceMode": 40,
            "interfaceType": 40
          }
        }
    },
    {
      "factor": {
        "profileID": 19
      },
      "profile": {
        "speed": 50000,
        "iphy": {
          "numLanes": 2,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 11,
          "interfaceType": 11
        }
      }
    },
    {
      "factor": {
        "profileID": 22
      },
      "profile": {
        "speed": 100000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 528,
          "medium": 1,
          "interfaceMode": 12,
          "interfaceType": 12
        }
      }
    }
  ]
}
)";
} // namespace

namespace facebook {
namespace fboss {
Morgan800ccPlatformMapping::Morgan800ccPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

Morgan800ccPlatformMapping::Morgan800ccPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace fboss
} // namespace facebook